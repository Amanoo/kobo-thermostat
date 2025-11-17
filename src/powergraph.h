#pragma once


#include <QWidget>
#include <QVector>


class PowerGraph : public QWidget {
Q_OBJECT
public:
explicit PowerGraph(QWidget* parent = nullptr);
void setPoints(const QVector<int>& pts);


protected:
void paintEvent(QPaintEvent* ev) override;


private:
QVector<int> m_points;
};
