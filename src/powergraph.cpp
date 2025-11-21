// powergraph.cpp
#include "powergraph.h"

#include <QPainter>
#include <QPaintEvent>
#include <QPainterPath>
#include <QPalette>
#include <algorithm>

PowerGraph::PowerGraph(QWidget *parent)
: QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAttribute(Qt::WA_TranslucentBackground);  // real transparency
}

void PowerGraph::setPoints(const QVector<int> &pts)
{
    m_points = pts;
    update();
}

void PowerGraph::paintEvent(QPaintEvent *)
{
    if (m_points.size() < 2)
        return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    bool darkMode = palette().window().color().lightness() < 128;
    QColor lineColor = darkMode ? QColor("#DDDDDD") : QColor("#222222");

    // min/max calculation
    int minVal = 0;
    int maxVal = *std::max_element(m_points.cbegin(), m_points.cend());
    if (maxVal == 0) maxVal = 1;
    if (maxVal < 100)
        maxVal = 100;

    const int margin = 10;
    QRectF area = rect().adjusted(margin, margin, -margin, -margin);

    // faint border so you can still see the bounds on transparent bg
    QPen borderPen(darkMode ? QColor(255,255,255,40) : QColor(0,0,0,60));
    p.setPen(borderPen);
    p.drawRect(area);

    // --- peak label ---------------------------------------------------------
    QFont peakFont = p.font();
    peakFont.setPointSize(10);
    p.setFont(peakFont);
    p.setPen(darkMode ? QColor("#DDDDDD") : QColor("#333333"));

    QString peakText = QString::number(maxVal) + " W";
    // small rect near top‑right inside the graph area
    QRectF labelRect(area.right() - 80, area.top(), 75, 18);
    p.drawText(labelRect, Qt::AlignRight | Qt::AlignVCenter, peakText);

    QPainterPath path;               // this now compiles because of the includes above
    int n = m_points.size();

    for (int i = 0; i < n; ++i) {
        qreal x = area.left() + i * area.width() / qreal(n - 1);
        qreal norm = (m_points[i] - minVal) / qreal(maxVal - minVal);
        qreal y = area.bottom() - norm * area.height();

        if (i == 0)
            path.moveTo(x, y);
        else
            path.lineTo(x, y);
    }

    p.setPen(QPen(lineColor, 2.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);
}
