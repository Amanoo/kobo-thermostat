#pragma once


#include <QMainWindow>


class Backend;
class UsageBar;
class PowerGraph;
class ThermostatDial;


#include <QLabel>


class MainWindow : public QMainWindow {
Q_OBJECT
public:
explicit MainWindow(Backend* backend, QWidget* parent = nullptr);


private:
Backend* m_backend;


// Left side widgets
QLabel* m_timeLabel;
QLabel* m_dateLabel;


UsageBar* m_gasBar;
QLabel* m_gasLabel;


PowerGraph* m_powerGraph;


UsageBar* m_powerBar;
QLabel* m_powerLabel;


// Right side
ThermostatDial* m_thermo;
};