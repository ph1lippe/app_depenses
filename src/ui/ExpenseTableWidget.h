#ifndef EXPENSETABLEWIDGET_H
#define EXPENSETABLEWIDGET_H

#include <QTableWidget>

class ExpenseTableWidget : public QTableWidget {
    Q_OBJECT

public:
    explicit ExpenseTableWidget(QWidget* parent = nullptr);
    int expenseIndexAtRow(int row) const;

signals:
    void rowSelectionChanged(int row);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
};

#endif // EXPENSETABLEWIDGET_H
