#include "powergraph.h"
#include <QPainter>
#include <QPaintEvent>
#include <QPainterPath>
#include <algorithm>


PowerGraph::PowerGraph(QWidget* parent) : QWidget(parent) {
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}


void PowerGraph::setPoints(const QVector<int>& pts) {
    m_points = pts;
    update();
}


void PowerGraph::paintEvent(QPaintEvent* ev) {
    Q_UNUSED(ev);
    QPainter p(this);
    p.fillRect(rect(), Qt::white);
    if (m_points.size() < 2) return;


    int mn = *std::min_element(m_points.constBegin(), m_points.constEnd());
    int mx = *std::max_element(m_points.constBegin(), m_points.constEnd());
    if (mx <= mn) mx = mn + 1;


    int margin = 8;
    QRectF box = rect().adjusted(margin, margin, -margin, -margin);


    p.setPen(QPen(Qt::black, 1));
    p.drawRect(box);


    QPainterPath path;
    int n = m_points.size();
    for (int i=0;i<n;i++) {
        double x = box.left() + double(i) * box.width() / (n - 1);
        double y = box.bottom() - box.height() * ((m_points[i] - mn) / double(mx - mn));
        if (i==0) path.moveTo(x,y); else path.lineTo(x,y);
    }
    p.drawPath(path);
}
