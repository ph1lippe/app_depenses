#ifndef MONTHFILTERWIDGET_H
#define MONTHFILTERWIDGET_H

#include <QWidget>

class QLabel;
class QSlider;
class QSpinBox;

class MonthFilterWidget : public QWidget {
    Q_OBJECT

public:
    explicit MonthFilterWidget(QWidget* parent = nullptr);
    int month() const;
    int year() const;
    void setMonth(int month);
    void setYear(int year);

signals:
    void filterChanged(int month, int year);

private slots:
    void onFilterValueChanged(int value);

private:
    void updateLabel();

    QLabel* monthLabel;
    QSlider* monthSlider;
    QSpinBox* yearSpinBox;
};

#endif // MONTHFILTERWIDGET_H
