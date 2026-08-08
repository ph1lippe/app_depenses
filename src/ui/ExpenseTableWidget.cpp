#include "ExpenseTableWidget.h"

#include <QApplication>
#include <QHeaderView>
#include <QKeyEvent>

ExpenseTableWidget::ExpenseTableWidget(QWidget* parent)
    : QTableWidget(parent) {
    setColumnCount(7);
    setHorizontalHeaderLabels({"Date", "Equal split", "Amount", "Cardholder", "Paid by", "Paid for", "Item"});
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setTabKeyNavigation(false);
    setFocusPolicy(Qt::StrongFocus);
    setAlternatingRowColors(true);
    horizontalHeader()->setStretchLastSection(true);
    setSortingEnabled(true);
    installEventFilter(this);

    connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, [this](const QItemSelection&, const QItemSelection&) {
        const QModelIndexList selectedRows = selectionModel()->selectedRows(0);
        if (!selectedRows.isEmpty()) {
            emit rowSelectionChanged(selectedRows.first().row());
        }
    });
}

bool ExpenseTableWidget::eventFilter(QObject* watched, QEvent* event) {
    if (watched == this && event->type() == QEvent::KeyPress) {
        const auto* keyEvent = static_cast<const QKeyEvent*>(event);
        if (keyEvent->modifiers() == Qt::ControlModifier && keyEvent->key() == Qt::Key_A) {
            selectAll();
            return true;
        }
    }
    return QTableWidget::eventFilter(watched, event);
}

int ExpenseTableWidget::expenseIndexAtRow(int row) const {
    if (row < 0 || row >= rowCount()) {
        return -1;
    }
    const QTableWidgetItem* item = this->item(row, 0);
    return item ? item->data(Qt::UserRole).toInt() : -1;
}
