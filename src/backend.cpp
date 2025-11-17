#include "backend.h"
#include <QJsonDocument>
#include <QJsonObject>


Backend::Backend(QObject* parent)
: QObject(parent)
{
// clock timer
connect(&m_clock, &QTimer::timeout, this, [this]{ emit timeChanged(); });
m_clock.start(1000);


// MQTT minimal
m_mqtt = new QMqttClient(this);
m_mqtt->setHostname("192.168.1.10");
m_mqtt->setPort(1883);
connect(m_mqtt, &QMqttClient::messageReceived, this, &Backend::onMessage);
m_mqtt->connectToHost();
m_mqtt->subscribe(QMqttTopicFilter("homeassistant/power"));
m_mqtt->subscribe(QMqttTopicFilter("homeassistant/gas"));
m_mqtt->subscribe(QMqttTopicFilter("homeassistant/thermostat/state"));
}


QString Backend::time() const { return QDateTime::currentDateTime().toString("HH:mm"); }
QString Backend::date() const { return QDateTime::currentDateTime().toString("ddd dd MMM yyyy"); }


double Backend::setpoint() const { return m_setpoint; }
double Backend::currentTemp() const { return m_current; }


QVector<int> Backend::powerMinutes() const {
QVector<int> out;
out.reserve(int(m_points.size()));
for (const auto &p : m_points) out.append(p.avgW);
return out;
}


void Backend::onMessage(const QByteArray& payload, const QMqttTopicName& topic) {
const QString t = topic.name();
if (t.contains("power")) {
bool ok=false; int w = int(QString::fromUtf8(payload).toDouble(&ok)+0.5);
if (ok) {
emit powerNowChanged(w);
addInstantPower(w, QDateTime::currentMSecsSinceEpoch());
}
} else if (t.contains("gas")) {
bool ok=false; double g = QString::fromUtf8(payload).toDouble(&ok);
if (ok) emit gasTodayChanged(g);
} else if (t.contains("thermostat/state")) {
const QJsonDocument doc = QJsonDocument::fromJson(payload);
if (doc.isObject()) {
auto o = doc.object();
m_current = o.value("current_temperature").toDouble();
m_setpoint = o.value("temperature").toDouble();
emit thermostatChanged();
}
}
}


void Backend::addInstantPower(int w, qint64 tsMs) {
if (m_bucketStart == 0) m_bucketStart = (tsMs / kBucketMs) * kBucketMs;
while (tsMs >= m_bucketStart + kBucketMs) {
flushBucket();
m_bucketStart += kBucketMs;
}
m_sum += w;
++m_count;
}


void Backend::flushBucket() {
int avg = (m_count>0) ? int(m_sum / m_count) : (m_points.empty()?0:m_points.back().avgW);
m_points.push_back({m_bucketStart, avg});
if ((int)m_points.size() > kMaxPts) m_points.pop_front();
m_sum = 0; m_count = 0;
emit powerSeriesChanged();
}