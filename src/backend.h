#pragma once


#include <QObject>
#include <QtMqtt/QMqttClient>
#include <QTimer>
#include <QDateTime>
#include <deque>
#include <QVector>


class Backend : public QObject {
Q_OBJECT
public:
explicit Backend(QObject* parent = nullptr);


QString time() const;
QString date() const;


double setpoint() const;
double currentTemp() const;


QVector<int> powerMinutes() const; // minute-bucketed averages


signals:
void timeChanged();
void powerNowChanged(int watts);
void gasTodayChanged(double m3);
void powerSeriesChanged();
void thermostatChanged();


public slots:


private slots:
void onMessage(const QByteArray& payload, const QMqttTopicName& topic);


private:
// Lightweight minute-averaging ring buffer (240 samples)
struct Pt { qint64 tMs; int avgW; };
static constexpr int kBucketMs = 60*1000;
static constexpr int kMaxPts = 240;
std::deque<Pt> m_points;
qint64 m_bucketStart = 0;
qint64 m_sum = 0;
int m_count = 0;


void addInstantPower(int w, qint64 tsMs);
void flushBucket();


QMqttClient* m_mqtt = nullptr;
QTimer m_clock;


double m_setpoint = 21.0;
double m_current = 20.0;
};