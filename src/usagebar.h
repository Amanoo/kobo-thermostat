#pragma once


#include <QWidget>

class UsageBar : public QWidget {
    Q_OBJECT
public:
    explicit UsageBar(QWidget* parent = nullptr);
    void setValue(double v); // 0..1

    // Add this line:
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* ev) override;

private:
    double m_value = 0.0;
};
