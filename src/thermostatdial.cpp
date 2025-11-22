#include "thermostatdial.h"
#include <QPainter>
#include <QMouseEvent>
#include <QtMath>
#include <QFontMetrics>

ThermostatDial::ThermostatDial(QWidget* parent)
: QWidget(parent)
{
    m_ignoreTimer.setSingleShot(true);
    connect(&m_ignoreTimer, &QTimer::timeout, this, [this]() {
        m_ignoreFromHA = false;
    });
}

void ThermostatDial::setSetpoint(double v)
{
    // Completely ignore updates from Home Assistant shortly after user interaction
    if (m_ignoreFromHA)
        return;

    // Only ignore while actively dragging (old behaviour stays for safety)
    if (m_ignoreSetpointUpdates)
        return;

    if (!qFuzzyCompare(v, m_setpoint)) {
        m_setpoint = v;
        update();
    }
}

void ThermostatDial::setCurrent(double v)
{
    m_current = v;
    update();   // current temp is always allowed
}

QSize ThermostatDial::sizeHint() const
{
    // Large default size so the circular layout has room to breathe.
    return QSize(550, 550);
}

void ThermostatDial::paintEvent(QPaintEvent* ev)
{
    Q_UNUSED(ev);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    QRectF r = rect();
    double side = r.width() * 0.94;
    QPointF center(r.width() / 2.0,
                   r.height() / 2.0 + side * 0.06);

    double thickness = side * 0.088;
    double radius = side / 2.0 - thickness / 2.0;

    const bool dark = palette().window().color().lightness() < 128;

    // --- Background arc -----------------------------------------------------
    QColor bgColor = dark ? QColor("#555555") : QColor("#888888");

    QPen bgPen(bgColor);
    bgPen.setWidthF(thickness);
    bgPen.setCapStyle(Qt::RoundCap);
    p.setPen(bgPen);

    p.drawArc(QRectF(center.x() - radius, center.y() - radius,
                     radius * 2, radius * 2),
                     225 * 16,         // arc start (degrees * 16)
                    -270 * 16);        // CCW sweep

    // --- Setpoint arc -------------------------------------------------------
    double frac = qBound(0.0, (m_setpoint - 10.0) / 20.0, 1.0);
    if (frac > 0.0) {
        QPen fgPen(dark ? Qt::lightGray : Qt::black);
        fgPen.setWidthF(thickness);
        fgPen.setCapStyle(Qt::RoundCap);
        p.setPen(fgPen);

        p.drawArc(QRectF(center.x() - radius, center.y() - radius,
                         radius * 2, radius * 2),
                         225 * 16,
                        -270 * frac * 16);
    }

    // --- Large setpoint text ------------------------------------------------
    QFont big;
    big.setPixelSize(qRound(side * 0.135));
    big.setBold(true);
    p.setFont(big);
    p.setPen(dark ? QColor("#EEEEEE") : Qt::black);


    QRectF setRect(0, r.height() * 0.34,
                   r.width(), r.height() * 0.3);

    p.drawText(setRect, Qt::AlignCenter,
               QString::number(m_setpoint, 'f', 1) + "°C");

    // --- “Now” temperature --------------------------------------------------
    QFont small;
    small.setPixelSize(qRound(side * 0.068));
    p.setFont(small);
    p.setPen(dark ? QColor("#DDDDDD") : QColor("#333333"));

    QRectF nowRect(0, r.height() * 0.48,
                   r.width(), r.height() * 0.3);

    p.drawText(nowRect, Qt::AlignCenter,
               QString("Nu %1°C").arg(QString::number(m_current, 'f', 1)));

    // --- Flame icon (displayed only when heating is off) --------------------
    if (m_heating) {
        QFont flameFont;
        flameFont.setPixelSize(qRound(side * 0.20));
        flameFont.setFamily(QStringLiteral("Segoe UI Emoji"));
        p.setFont(flameFont);

        p.drawText(QRectF(center.x() - radius * 0.8,
                          center.y() + radius * 0.25,
                          radius * 1.6, radius * 0.8),
                   Qt::AlignCenter,
                   QStringLiteral("🔥"));
    }
}

void ThermostatDial::mousePressEvent(QMouseEvent* ev)
{
    if (ev->buttons() & Qt::LeftButton) {
        m_ignoreSetpointUpdates = true;
        m_finalSetpointToSend = m_setpoint;  // initialise
        updateFromPos(ev->pos());
    }
}

void ThermostatDial::mouseMoveEvent(QMouseEvent* ev)
{
    if (ev->buttons() & Qt::LeftButton) {
        updateFromPos(ev->pos());  // only visual + store final value
    }
}

void ThermostatDial::mouseReleaseEvent(QMouseEvent* ev)
{
    if (ev->button() == Qt::LeftButton) {
        m_ignoreSetpointUpdates = false;
        updateFromPos(ev->pos());   // in case mouse left the arc

        // ←←← NOW we send exactly ONE request with the final temperature
        if (m_finalSetpointToSend >= 0.0) {
            emit setpointEdited(m_finalSetpointToSend);
        }

        // Block HA echo for 1.8 seconds so the dial never jumps back
        m_ignoreFromHA = true;
        m_ignoreTimer.start(1800);
    }
    QWidget::mouseReleaseEvent(ev);
}

void ThermostatDial::tabletEvent(QTabletEvent* ev)
{
    if (ev->type() == QEvent::TabletRelease)
        m_ignoreSetpointUpdates = false;
}

// Maps a mouse position to a temperature value using the same angle rules
// as the original QML version.
void ThermostatDial::updateFromPos(const QPoint& p)
{
    const double cx = width()  / 2.0;
    const double cy = height() / 2.0;

    const double dx = p.x() - cx;
    const double dy = p.y() - cy;

    // Convert (dx, dy) into an angle:
    // QML uses atan2 with Y inverted, then rotates so that 0° is at the right.
    double ang = std::atan2(-dy, dx) * 180.0 / M_PI;
    ang = 90.0 - ang;
    if (ang < 0)
        ang += 360.0;

    // Arc runs from 225° through 270° of sweep (ends effectively at 45°)
    double a = ang;
    const double start = 225.0;
    const double end   = start + 270.0;

    if (a < start)
        a += 360.0;

    if (a >= start && a <= end) {
        double frac = (a - start) / 270.0;
        double t = 10.0 + frac * 20.0;   // 10 → 30°C
        double newSp = qBound(10.0, t, 30.0);

        if (!qFuzzyCompare(newSp, m_setpoint)) {
            m_setpoint = newSp;
            m_finalSetpointToSend = newSp;   // ← remember for release
            update();                        // visual feedback only
        }
    }
}
