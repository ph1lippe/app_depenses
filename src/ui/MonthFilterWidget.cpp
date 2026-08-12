#include "MonthFilterWidget.h"

#include <QDate>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>

MonthFilterWidget::MonthFilterWidget(QWidget* parent)
    : QWidget(parent),
      monthLabel(new QLabel(this)),
      monthSlider(new QSlider(Qt::Horizontal, this)),
      yearSpinBox(new QSpinBox(this)) {
    QHBoxLayout* monthFilterLayout = new QHBoxLayout(this);

    monthSlider->setRange(1, 12);
    monthSlider->setValue(1);
    monthSlider->setTickPosition(QSlider::TicksBelow);
    monthSlider->setTickInterval(1);
    monthSlider->setMinimumWidth(320);
    monthSlider->setFixedHeight(18);
    monthSlider->setStyleSheet(
        "QSlider::groove:horizontal { height: 10px; margin: 0px; }"
        "QSlider::handle:horizontal { width: 32px; height: 32px; margin: -11px 0; }"
        "QSlider::sub-page:horizontal, QSlider::add-page:horizontal { height: 10px; }"
    );

    yearSpinBox->setRange(2000, 2100);
    yearSpinBox->setValue(QDate::currentDate().year());
    yearSpinBox->setFixedHeight(24);

    monthFilterLayout->addWidget(new QLabel("Month", this));
    monthFilterLayout->addWidget(monthSlider);
    monthFilterLayout->addWidget(new QLabel("Year", this));
    monthFilterLayout->addWidget(yearSpinBox);
    monthFilterLayout->addWidget(monthLabel);
    monthFilterLayout->addStretch();

    connect(monthSlider, &QSlider::valueChanged, this, &MonthFilterWidget::onFilterValueChanged);
    connect(yearSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MonthFilterWidget::onFilterValueChanged);

    updateLabel();
}

int MonthFilterWidget::month() const {
    return monthSlider->value();
}

int MonthFilterWidget::year() const {
    return yearSpinBox->value();
}

void MonthFilterWidget::setMonth(int month) {
    monthSlider->setValue(month);
    updateLabel();
}

void MonthFilterWidget::setYear(int year) {
    yearSpinBox->setValue(year);
    updateLabel();
}

void MonthFilterWidget::onFilterValueChanged(int) {
    updateLabel();
    emit filterChanged(month(), year());
}

void MonthFilterWidget::updateLabel() {
    static const QStringList monthNames = {
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };
    const int currentMonth = month();
    const QString monthName = monthNames.value(currentMonth - 1, "Unknown");
    monthLabel->setText(QString("Showing %1 %2").arg(monthName).arg(year()));
}
