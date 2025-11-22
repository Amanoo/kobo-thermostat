#pragma once

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QVector>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class Backend : public QObject
{
    Q_OBJECT
public:
    explicit Backend(QObject* parent = nullptr);

    QString time() const;
    QString date() const;

    double setpoint() const { return m_setpoint; }
    double currentTemp() const { return m_current; }
    QVector<int> powerMinutes() const { return m_powerMinutes; }  // last 240 minutes
    bool heatingActive() const { return m_heatingActive; }

signals:
    void timeChanged();
    void powerNowChanged(int watts);
    void gasTodayChanged(double m3);
    void powerSeriesChanged();
    void thermostatChanged();

public slots:
    void setThermostatSetpoint(double temperature);

private slots:
    void pollData();

private:
    void fetchStates();
    void fetchPowerHistory();

    // ────── CONFIGURE THESE THREE LINES ONLY ──────
    const QUrl m_baseUrl = QUrl("http://192.168.1.11:8123");
    const QString m_token = "YOUR TOKEN";
    // ───────────────────────────────────────────────

    const QString m_powerEntity   = "sensor.dsmr_reader_power_consumed";
    const QString m_gasEntity     = "sensor.daily_gas";
    const QString m_climateEntity = "climate.toon_thermostat_3";

    QTimer m_powerGraphUpdateTimer;   // NEW: controls UI refresh rate
    bool m_firstPowerHistory = true;  // force immediate update on first fetch

    QNetworkAccessManager* m_nam = nullptr;
    QTimer m_clockTimer;
    QTimer m_pollTimer;

    double m_setpoint;
    double m_current;

    QVector<int> m_powerMinutes;      // 360 entries = last 3 hours (30s per sample)
    QDate m_gasReferenceDate;
    double m_gasAtStartOfDay = 0.0;
    double m_gasToday = 0.0;

    bool m_heatingActive = false;
};
