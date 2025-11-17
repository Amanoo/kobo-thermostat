#pragma once


#include <QWidget>

class ThermostatDial : public QWidget {
    Q_OBJECT
public:
    explicit ThermostatDial(QWidget* parent = nullptr);

    double setpoint() const { return m_setpoint; }
    double current() const { return m_current; }
    void setHeatingActive(bool a) { m_heating = a; update(); }

    // Add this line:
    QSize sizeHint() const override;

signals:
    void setpointEdited(double v);

public slots:
    void setSetpoint(double v) { m_setpoint = v; update(); }
    void setCurrent(double v) { m_current = v; update(); }

protected:
    void paintEvent(QPaintEvent* ev) override;
    void mousePressEvent(QMouseEvent* ev) override;
    void mouseMoveEvent(QMouseEvent* ev) override;

private:
    void updateFromPos(const QPoint& p);
    double m_setpoint = 21.0;
    double m_current = 20.0;
    bool m_heating = false;
};
