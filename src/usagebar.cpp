// usagebar.cpp
#include "usagebar.h"
#include <QPainter>
#include <QPaintEvent>

UsageBar::UsageBar(QWidget* parent) : QWidget(parent)
{
    // Very important: we want the bar to expand vertically but stay narrow
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
}

QSize UsageBar::sizeHint() const
{
    return QSize(70, 225);       // Exact Toon width & reasonable height
}

void UsageBar::setValue(double v)
{
    v = qBound(0.0, v, 1.0);
    if (!qFuzzyCompare(v + 1.0, m_value + 1.0)) {
        m_value = v;
        update();
    }
}

void UsageBar::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Inner area — a bit of padding on the sides, more on top/bottom for the rounded look
    QRect r = rect().adjusted(1, 1, -1, -1);

    // 1. Dark transparent background well (exact Toon color & opacity)
    p.setBrush(QColor(0, 0, 0, 28));
    p.setPen(QPen(QColor(0, 0, 0, 90), 1));
    p.drawRoundedRect(r, 22, 22);

    // 2. Black fill bar — grows from the bottom
    if (m_value > 0.001) {
        int h = int(r.height() * m_value);
        QRect fill = r;
        fill.setHeight(h);
        fill.moveTop(r.bottom() - h);

        // Slightly narrower than the well → classic Toon look
        fill.adjust(8, 0, -8, 0);

        p.setBrush(Qt::black);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(fill, 14, 14);
    }
}
