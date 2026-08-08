#ifndef SETTLEMENTRESULTWIDGET_H
#define SETTLEMENTRESULTWIDGET_H

#include <QWidget>

class QTextEdit;

class SettlementResultWidget : public QWidget {
    Q_OBJECT

public:
    explicit SettlementResultWidget(QWidget* parent = nullptr);
    void setResultText(const QString& text);

private:
    QTextEdit* resultArea;
};

#endif // SETTLEMENTRESULTWIDGET_H
