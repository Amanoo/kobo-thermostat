// usagebar.cpp
#include "usagebar.h"
#include <QPainter>
#include <QPaintEvent>
#include <QPainterPath>
#include <QtMath>

UsageBar::UsageBar(QWidget* parent) : QWidget(parent)
{
    // Very important: we want the bar to expand vertically but stay narrow
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
}

QSize UsageBar::sizeHint() const
{
    return QSize(70, 245);       // Exact Toon width & reasonable height
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
    bool dark = palette().window().color().lightness() < 128;
    QRect r = rect().adjusted(1, 1, -1, -1);

    // 1. Outer well — identical to real Toon
    QColor wellColor   = dark ? QColor(0, 0, 0, 45)  : QColor(0, 0, 0, 28);
    QColor borderColor = dark ? QColor(105, 105, 105, 255)
                              : QColor(145, 145, 145, 255);

    p.setBrush(wellColor);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(r, 22, 22);

    // 2. Inner visible fill (only if value is non‑trivial)
    if (m_value >= 0.01) {
        QRectF inner = r.adjusted(2, 2, -2, -2);

        // Fill color
        QColor fillColor = dark ? QColor("#E8E8E8") : QColor("#1A1A1A");

        // Special case: treat "almost 100%" as completely full
        if (m_value >= 0.999) {
            // Make it 2 px taller and shift 1 px so it visually touches both ends
            // horizontally still widened by 1 px on each side
            QRectF full = inner.adjusted(-1, -1, 1, 1);   // +2 height, +2 width
            qreal radius = qMin(qreal(20), full.height() / 2.0);

            p.setPen(Qt::NoPen);
            p.setBrush(fillColor);
            p.drawRoundedRect(full, radius, radius);
        } else {
            int h = qCeil(inner.height() * m_value);
            if (h > 0) {
                QRectF clipRect = inner.adjusted(-1, 0, 1, 2);
                qreal clipRadius = qMin(qreal(20), clipRect.height() / 2.0);

                QPainterPath clipPath;
                clipPath.addRoundedRect(clipRect, clipRadius, clipRadius);

                p.save();
                p.setClipPath(clipPath);

                QRectF fill = inner;
                fill.setHeight(h);
                fill.adjust(-1, 0, 1, 0);
                fill.moveTop(inner.bottom() - h + 1);

                qreal radius = qMin(qreal(20), fill.height() / 2.0);

                p.setPen(Qt::NoPen);
                p.setBrush(fillColor);
                p.drawRoundedRect(fill, radius, radius);

                p.restore();
            }
        }
    }

    // 3. Border is always drawn
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(borderColor, 2));
    p.drawRoundedRect(r, 22, 22);
}
