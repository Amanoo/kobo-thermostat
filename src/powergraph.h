#pragma once

#include <QWidget>
#include <QVector>

// Forward declare only what we really need, but pull in QPainterPath here too
#include <QPainterPath>

class PowerGraph : public QWidget
{
    Q_OBJECT
public:
    explicit PowerGraph(QWidget *parent = nullptr);
    void setPoints(const QVector<int> &pts);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QVector<int> m_points;
};
