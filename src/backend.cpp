#include "backend.h"

//#include <qfile.h>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QNetworkRequest>
//#include <QDir>

Backend::Backend(QObject* parent)
: QObject(parent)
{
    m_nam = new QNetworkAccessManager(this);

    connect(&m_clockTimer, &QTimer::timeout, this, &Backend::timeChanged);
    m_clockTimer.start(1000);

    connect(&m_pollTimer, &QTimer::timeout, this, &Backend::pollData);
    m_pollTimer.start(3000);

    pollData();

    // ←←← Debounce timer completely removed
}

QString Backend::time() const { return QDateTime::currentDateTime().toString("HH:mm"); }
QString Backend::date() const { return QDateTime::currentDateTime().toString("ddd dd MMM yyyy"); }

void Backend::pollData()
{
    fetchStates();
    fetchPowerHistory();
}

void Backend::setThermostatSetpoint(double temperature)
{
    QNetworkRequest req;
    req.setRawHeader("Authorization", ("Bearer " + m_token).toUtf8());
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QUrl url = m_baseUrl.resolved(QUrl("/api/services/climate/set_temperature"));
    req.setUrl(url);

    QJsonObject json;
    json["entity_id"] = m_climateEntity;
    json["temperature"] = temperature;

    QNetworkReply* reply = m_nam->post(req, QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, this, [reply]() {
        reply->deleteLater();
    });
}

void Backend::fetchStates()
{
    QNetworkRequest baseReq;
    baseReq.setRawHeader("Authorization", ("Bearer " + m_token).toUtf8());
    baseReq.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    auto getState = [this, baseReq](const QString& entity) mutable {
        QUrl url = m_baseUrl.resolved(QUrl("/api/states/" + entity));
        QNetworkRequest req = baseReq;
        req.setUrl(url);

        QNetworkReply* reply = m_nam->get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply, entity]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                qWarning() << "HA error:" << reply->errorString();
                return;
            }

            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            if (!doc.isObject()) return;

            QJsonObject obj = doc.object();
            QString state = obj["state"].toString();

            if (entity == m_powerEntity) {
                bool ok = false;
                double kw = state.toDouble(&ok);
                if (ok) emit powerNowChanged(qRound(kw * 1000));
            }
            else if (entity == m_gasEntity) {
                bool ok = false;
                double usageToday = state.toDouble(&ok);
                if (!ok) return;

                if (!qFuzzyCompare(usageToday, m_gasToday)) {
                    m_gasToday = usageToday;
                    emit gasTodayChanged(usageToday);
                }
            }
            else if (entity == m_climateEntity) {
                QJsonObject attr = obj["attributes"].toObject();
                double sp  = attr["temperature"].toDouble(m_setpoint);
                double cur = attr["current_temperature"].toDouble(m_current);

                bool changed = false;
                if (!qFuzzyCompare(sp, m_setpoint)) { m_setpoint = sp; changed = true; }
                if (!qFuzzyCompare(cur, m_current))  { m_current  = cur; changed = true; }
                if (changed) emit thermostatChanged();
            }
        });
    };

    getState(m_powerEntity);
    getState(m_gasEntity);
    getState(m_climateEntity);
}

void Backend::fetchPowerHistory()
{
    QUrl url = m_baseUrl.resolved(QUrl("/api/history/period"));
    QUrlQuery query;
    query.addQueryItem("filter_entity_id", m_powerEntity);
    query.addQueryItem("minimal_response", "");
    query.addQueryItem("significant_changes_only", "false");

    // --- Time window and binning configuration ------------------------------
    constexpr int WINDOW_MINUTES      = 360;        // last 3 hours
    constexpr int WINDOW_SECONDS      = WINDOW_MINUTES * 60;
    constexpr int BIN_SECONDS         = 30;         // 30s per bucket
    constexpr int TOTAL_BINS          = WINDOW_SECONDS / BIN_SECONDS; // 360

    QDateTime start = QDateTime::currentDateTimeUtc().addSecs(-WINDOW_SECONDS);
    start.setTime(QTime(start.time().hour(), start.time().minute(), 0, 0));

    QDateTime end = QDateTime::currentDateTimeUtc();
    end.setTime(QTime(end.time().hour(), end.time().minute(), 0, 0));

    query.addQueryItem("start_time", start.toString(Qt::ISODate));
    query.addQueryItem("end_time",   QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    url.setQuery(query);

    QNetworkRequest req(url);
    req.setRawHeader("Authorization", ("Bearer " + m_token).toUtf8());

    QNetworkReply* reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "History error:" << reply->errorString();
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonArray entities = doc.array();
        if (entities.isEmpty()) return;

        QJsonArray history = entities[0].toArray();

        // --- 30-second buckets ------------------------------------------------
        constexpr int WINDOW_MINUTES = 180;
        constexpr int WINDOW_SECONDS = WINDOW_MINUTES * 60;
        constexpr int BIN_SECONDS    = 60;
        constexpr int TOTAL_BINS     = WINDOW_SECONDS / BIN_SECONDS; // 360

        QVector<qint64> sums(TOTAL_BINS, 0);
        QVector<int>    counts(TOTAL_BINS, 0);

        QDateTime nowUtc = QDateTime::currentDateTimeUtc();

        for (const QJsonValue& v : history) {
            QJsonObject o = v.toObject();

            QString s = o["state"].toString();
            if (s.isEmpty())
                continue;

            double kw = s.toDouble();
            int watts = qRound(kw * 1000);

            QString tsStr = o["last_updated"].toString();
            if (tsStr.isEmpty())
                tsStr = o["last_changed"].toString();
            if (tsStr.isEmpty())
                continue;

            QDateTime ts = QDateTime::fromString(tsStr, Qt::ISODateWithMs);
            bool tsOk = ts.isValid();
            if (tsOk)
                ts = ts.toUTC();

            if (!tsOk)
                continue;

            qint64 secsAgo = ts.secsTo(nowUtc);
            if (secsAgo < 0 || secsAgo >= WINDOW_SECONDS)
                continue;   // outside the 3h window

            int bin = static_cast<int>(secsAgo / BIN_SECONDS);   // 0..359 (newest: small bin)
            if (bin < 0 || bin >= TOTAL_BINS)
                continue;

            // Reverse so oldest is left, newest is right
            int idx = TOTAL_BINS - 1 - bin;                      // 0..359
            sums[idx]   += watts;
            counts[idx] += 1;
        }

        // --- Build final 360-point series (average per bucket) ---------------
        QVector<int> result(TOTAL_BINS);
        for (int i = 0; i < TOTAL_BINS; ++i) {
            result[i] = counts[i] > 0
                        ? qRound(static_cast<double>(sums[i]) / counts[i])
                        : 0;
        }

        // --- Optional: log the final series to CSV ---------------------------
        /**QString logPath = QDir::homePath() + "/power_history_debug.csv";
        QFile logFile(logPath);
        if (logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream log(&logFile);
            log << "index,watts\n";
            for (int i = 0; i < result.size(); ++i) {
                log << i << ',' << result[i] << '\n';
            }
            logFile.flush();
            logFile.close();
        }**/

        // --- Push to UI only if changed --------------------------------------
        if (m_powerMinutes != result) {
            m_powerMinutes = result;
            emit powerSeriesChanged();
        }
    });
}