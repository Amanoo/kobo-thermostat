#pragma once

#include <QWidget>
#include <QTimer>          // ← add this

class ThermostatDial : public QWidget {
    Q_OBJECT
public:
    explicit ThermostatDial(QWidget* parent = nullptr);

    double setpoint() const { return m_setpoint; }
    double current() const { return m_current; }
    void setHeatingActive(bool a) { m_heating = a; update(); }

    // ←←← NEW: call this from MainWindow when a preset is clicked
    void startIgnoreFromHA() { m_ignoreFromHA = true; m_ignoreTimer.start(1800); }

    QSize sizeHint() const override;

signals:
    void setpointEdited(double v);

public slots:
    void setSetpoint(double v);
    void setCurrent(double v);

protected:
    void paintEvent(QPaintEvent* ev) override;
    void mousePressEvent(QMouseEvent* ev) override;
    void mouseMoveEvent(QMouseEvent* ev) override;
    void mouseReleaseEvent(QMouseEvent* ev) override;
    #ifndef Q_OS_ANDROID
    void tabletEvent(QTabletEvent* ev) override;
    #endif

private:
    void updateFromPos(const QPoint& p);

    double m_setpoint;
    double m_current;
    bool m_heating = false;
    bool m_ignoreSetpointUpdates = false;   // while dragging
    bool m_ignoreFromHA = false;             // ← NEW: block HA echo after user action

    QTimer m_ignoreTimer;                    // ← NEW timer

    double m_finalSetpointToSend;
};
