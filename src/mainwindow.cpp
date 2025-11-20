#include "mainwindow.h"
#include "backend.h"
#include "usagebar.h"
#include "powergraph.h"
#include "thermostatdial.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QFrame>
#include <QLabel>
#include <QSpacerItem>

MainWindow::MainWindow(Backend* backend, QWidget* parent)
: QMainWindow(parent), m_backend(backend)
{
    QWidget* central = new QWidget(this);
    setCentralWidget(central);

    QHBoxLayout* root = new QHBoxLayout(central);
    root->setSpacing(12);
    root->setContentsMargins(0, 0, 0, 0);

    // Card style used for the four left-side info panels
    const QString cardStyle =
    "QFrame {"
    "  background: transparent;"
    "  border: 2px solid #444444;"
    "  border-radius: 16px;"
    "}"
    "QLabel {"
    "  border: none;"
    "  background: transparent;"
    "  outline: none;"
    "}";

    auto makeCard = [cardStyle](QFrame* frame) {
        frame->setStyleSheet(cardStyle);
        frame->setFrameShape(QFrame::NoFrame);
    };

    // Reusable bold title label
    auto makeTitle = [this](const QString& text, QWidget* parent = nullptr) -> QLabel* {
        QLabel* label = new QLabel(text, parent);
        label->setAlignment(Qt::AlignCenter);
        QFont f = label->font();
        f.setPointSize(22);
        f.setBold(true);
        label->setFont(f);
        label->setContentsMargins(0, 0, 0, 8);
        return label;
    };

    // ------------------------------------------------------------------
    // Left side: 2×2 grid (clock, gas, power graph, current power)
    // ------------------------------------------------------------------
    QWidget* leftWidget = new QWidget(central);
    QGridLayout* leftLayout = new QGridLayout(leftWidget);
    leftLayout->setSpacing(12);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    // Clock
    QFrame* clockFrame = new QFrame(leftWidget);
    makeCard(clockFrame);
    QVBoxLayout* clockLayout = new QVBoxLayout(clockFrame);

    m_timeLabel = new QLabel(backend->time(), clockFrame);
    m_dateLabel = new QLabel(backend->date(), clockFrame);

    QFont timeFont = m_timeLabel->font();
    timeFont.setPointSize(80);
    timeFont.setBold(true);
    m_timeLabel->setFont(timeFont);
    m_timeLabel->setAlignment(Qt::AlignCenter);

    QFont dateFont = m_dateLabel->font();
    dateFont.setPointSize(24);
    m_dateLabel->setFont(dateFont);
    m_dateLabel->setAlignment(Qt::AlignCenter);

    clockLayout->insertStretch(0, 1);
    clockLayout->addWidget(m_timeLabel);
    clockLayout->addWidget(m_dateLabel);
    clockLayout->addStretch(1);
    leftLayout->addWidget(clockFrame, 0, 0);

    // Gas today
    QFrame* gasFrame = new QFrame(leftWidget);
    makeCard(gasFrame);
    QVBoxLayout* gasLayout = new QVBoxLayout(gasFrame);
    gasLayout->setSpacing(4);
    gasLayout->addWidget(makeTitle("Gas Today", gasFrame));
    gasFrame->setMaximumWidth(180);

    m_gasBar = new UsageBar(gasFrame);
    m_gasLabel = new QLabel("-- m³", gasFrame);
    m_gasLabel->setAlignment(Qt::AlignCenter);

    QFont gasLabelFont = m_gasLabel->font();
    gasLabelFont.setPointSize(22);
    gasLabelFont.setBold(true);
    m_gasLabel->setFont(gasLabelFont);

    gasLayout->addWidget(m_gasBar, 0, Qt::AlignCenter);
    gasLayout->addWidget(m_gasLabel);
    gasLayout->addStretch(1);
    leftLayout->addWidget(gasFrame, 0, 1);

    // Power graph (last 4 hours)
    QFrame* powerGraphFrame = new QFrame(leftWidget);
    makeCard(powerGraphFrame);
    QVBoxLayout* powerGraphLayout = new QVBoxLayout(powerGraphFrame);
    powerGraphLayout->setSpacing(4);
    powerGraphLayout->addWidget(makeTitle("Power (last 4 hours)", powerGraphFrame));

    m_powerGraph = new PowerGraph(powerGraphFrame);
    powerGraphLayout->addWidget(m_powerGraph, 1);
    leftLayout->addWidget(powerGraphFrame, 1, 0);

    // Current power
    QFrame* powerNowFrame = new QFrame(leftWidget);
    makeCard(powerNowFrame);
    QVBoxLayout* powerNowLayout = new QVBoxLayout(powerNowFrame);
    powerNowLayout->setSpacing(4);
    powerNowLayout->addWidget(makeTitle("Power Now", powerNowFrame));
    powerNowFrame->setMaximumWidth(180);

    m_powerBar = new UsageBar(powerNowFrame);
    m_powerLabel = new QLabel("-- W", powerNowFrame);
    m_powerLabel->setAlignment(Qt::AlignCenter);

    QFont powerLabelFont = m_powerLabel->font();
    powerLabelFont.setPointSize(22);
    powerLabelFont.setBold(true);
    m_powerLabel->setFont(powerLabelFont);

    powerNowLayout->addWidget(m_powerBar, 0, Qt::AlignCenter);
    powerNowLayout->addWidget(m_powerLabel);
    powerNowLayout->addStretch(1);
    leftLayout->addWidget(powerNowFrame, 1, 1);

    // Let all four cards grow equally
    leftLayout->setRowStretch(0, 1);
    leftLayout->setRowStretch(1, 1);
    leftLayout->setColumnStretch(0, 1);
    leftLayout->setColumnStretch(1, 1);

    root->addWidget(leftWidget, 1);

    // ------------------------------------------------------------------
    // Right side: thermostat dial + preset buttons
    // ------------------------------------------------------------------
    QWidget* rightWidget = new QWidget(central);
    QVBoxLayout* rightLay = new QVBoxLayout(rightWidget);
    rightLay->setSpacing(12);
    rightLay->setContentsMargins(0, 0, 0, 0);

    // Push dial down a bit
    rightLay->addSpacerItem(new QSpacerItem(0, 45, QSizePolicy::Minimum, QSizePolicy::Fixed));

    QFrame* tFrame = new QFrame(rightWidget);
    tFrame->setFrameShape(QFrame::StyledPanel);
    QVBoxLayout* tLay = new QVBoxLayout(tFrame);
    tLay->setContentsMargins(0, 0, 0, 0);

    m_thermo = new ThermostatDial(tFrame);
    tLay->addWidget(m_thermo, 0, Qt::AlignCenter);
    tFrame->setFixedHeight(410);
    rightLay->addWidget(tFrame);

    // Small gap before preset buttons
    rightLay->addSpacerItem(new QSpacerItem(0, 5, QSizePolicy::Minimum, QSizePolicy::Fixed));

    QFrame* presetsFrame = new QFrame(rightWidget);
    presetsFrame->setFrameShape(QFrame::StyledPanel);
    QGridLayout* presetsGrid = new QGridLayout(presetsFrame);
    presetsGrid->setHorizontalSpacing(22);
    presetsGrid->setVerticalSpacing(16);
    presetsGrid->setContentsMargins(11, 11, 11, 11);
    presetsFrame->setMaximumWidth(460);

    struct Preset { const char* name; int temp; } presets[] = {
        {"Away\n(15°C)", 15}, {"Sleep\n(17°C)", 17},
        {"Home\n(19°C)", 19}, {"Comfort\n(21°C)", 21}
    };

    int idx = 0;
    for (int r = 0; r < 2; ++r) {
        for (int c = 0; c < 2; ++c) {
            QPushButton* b = new QPushButton(presets[idx].name, presetsFrame);
            b->setMinimumHeight(100);

            QFont bf = b->font();
            bf.setPointSize(20);
            bf.setBold(true);
            b->setFont(bf);

            // Fixed the lambda syntax that was broken in your paste
            connect(b, &QPushButton::clicked, this, [this, temp = presets[idx].temp]() {
                emit m_thermo->setpointEdited(double(temp));
                m_thermo->setSetpoint(double(temp));
            });

            presetsGrid->addWidget(b, r, c);
            ++idx;
        }
    }

    rightLay->addWidget(presetsFrame);
    rightLay->addSpacerItem(new QSpacerItem(0, 40, QSizePolicy::Minimum, QSizePolicy::Fixed));
    root->addWidget(rightWidget, 0);

    // ------------------------------------------------------------------
    // Backend → UI connections
    // ------------------------------------------------------------------
    connect(backend, &Backend::timeChanged, this, [this]{
        m_timeLabel->setText(m_backend->time());
        m_dateLabel->setText(m_backend->date());
    });

    connect(backend, &Backend::powerNowChanged, this, [this](int w){
        m_powerLabel->setText(QString::number(w) + " W");
        m_powerBar->setValue(qMin(1.0, w / 4000.0));
    });

    connect(backend, &Backend::gasTodayChanged, this, [this](double m3){
        m_gasLabel->setText(QString::number(m3, 'f', 2) + " m³");
        m_gasBar->setValue(qMin(1.0, m3 / 5.0));
    });

    connect(backend, &Backend::powerSeriesChanged, this, [this]{
        m_powerGraph->setPoints(m_backend->powerMinutes());
    });

    connect(backend, &Backend::thermostatChanged, this, [this]{
        m_thermo->setSetpoint(m_backend->setpoint());
        m_thermo->setCurrent(m_backend->currentTemp());
    });

    // Optional: forward manual changes from dial back to backend
    connect(m_thermo, &ThermostatDial::setpointEdited,
            m_backend, &Backend::setThermostatSetpoint);

    setWindowTitle("Toon-like Dashboard (Qt Widgets)");
    resize(1024, 768);
}
