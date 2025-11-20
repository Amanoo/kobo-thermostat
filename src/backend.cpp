#include "backend.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QNetworkRequest>
#include <QDebug>

Backend::Backend(QObject* parent)
: QObject(parent)
{
    m_nam = new QNetworkAccessManager(this);

    connect(&m_clockTimer, &QTimer::timeout, this, &Backend::timeChanged);
    m_clockTimer.start(1000);

    connect(&m_pollTimer, &QTimer::timeout, this, &Backend::pollData);
    m_pollTimer.start(3000);



    pollData();

    m_setpointDebounceTimer.setSingleShot(true);
    m_setpointDebounceTimer.setInterval(600); // 600 ms after you stop dragging
    connect(&m_setpointDebounceTimer, &QTimer::timeout, this, [this]() {
        if (m_pendingSetpoint < 0.0) return;

        // ←←← THIS is where we actually send to Home Assistant
        QNetworkRequest req;
        req.setRawHeader("Authorization", ("Bearer " + m_token).toUtf8());
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QUrl url = m_baseUrl.resolved(QUrl("/api/services/climate/set_temperature"));
        req.setUrl(url);

        QJsonObject json;
        json["entity_id"] = m_climateEntity;
        json["temperature"] = m_pendingSetpoint;

        QNetworkReply* reply = m_nam->post(req, QJsonDocument(json).toJson());
        connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);

        m_pendingSetpoint = -1.0;
    });
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
    m_pendingSetpoint = temperature;
    m_setpointDebounceTimer.start();
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
    // ←←← FIXED: pass a QUrl, not a string
    QUrl url = m_baseUrl.resolved(QUrl("/api/history/period"));
    QUrlQuery query;
    query.addQueryItem("filter_entity_id", m_powerEntity);
    query.addQueryItem("minimal_response", "");
    query.addQueryItem("significant_changes_only", "false");

    QDateTime start = QDateTime::currentDateTimeUtc().addSecs(-4*3600 - 600);
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

        QVector<qint64> sums(240, 0);
        QVector<int>    counts(240, 0);

        for (const QJsonValue& v : history) {
            QJsonObject o = v.toObject();
            QString s = o["s"].toString();
            if (s.isEmpty()) continue;

            double kw = s.toDouble();
            int watts = qRound(kw * 1000);

            QDateTime ts = QDateTime::fromString(o["lc"].toString(), Qt::ISODateWithMs);
            if (!ts.isValid()) continue;

            qint64 minsAgo = ts.secsTo(QDateTime::currentDateTime()) / 60;
            if (minsAgo < 0 || minsAgo >= 240) continue;

            int idx = static_cast<int>(minsAgo);
            sums[idx] += watts;
            counts[idx]++;
        }

        QVector<int> result(240);
        for (int i = 0; i < 240; ++i) {
            // ←←← FIXED: cast to double to avoid qRound() ambiguity
            result[i] = counts[i] > 0 ? qRound(static_cast<double>(sums[i]) / counts[i]) : 0;
        }

        if (m_powerMinutes != result) {
            m_powerMinutes = result;
            emit powerSeriesChanged();
        }
    });
}
