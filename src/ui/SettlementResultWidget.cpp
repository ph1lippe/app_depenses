#include "SettlementResultWidget.h"

#include <QTextEdit>
#include <QVBoxLayout>

SettlementResultWidget::SettlementResultWidget(QWidget* parent)
    : QWidget(parent),
      resultArea(new QTextEdit(this)) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    resultArea->setReadOnly(true);
    resultArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    resultArea->setFixedHeight(resultArea->fontMetrics().lineSpacing() + 8);
    layout->addWidget(resultArea);
}

void SettlementResultWidget::setResultText(const QString& text) {
    resultArea->setPlainText(text);
}
