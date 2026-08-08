#include "RecurringExpensesDialog.h"
#include "userList.h"
#include "expenseList.h"
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QMessageBox>
#include <QHeaderView>

RecurringExpensesDialog::RecurringExpensesDialog(UserList& userList,
                                               ExpenseList& recurringExpenses,
                                               QWidget* parent)
    : QDialog(parent),
      m_userList(userList),
      m_recurringExpenses(recurringExpenses),
      table(new QTableWidget(this)),
      itemEdit(new QLineEdit(this)),
      amountEdit(new QDoubleSpinBox(this)),
      splitCheck(new QCheckBox("Equal split", this)),
      paidForCombo(new QComboBox(this)),
      paidByCombo(new QComboBox(this)),
      addButton(new QPushButton("Add", this)),
      removeButton(new QPushButton("Remove", this)),
      closeButton(new QPushButton("Close", this)) {
    setWindowTitle("Recurring expenses");
    resize(600, 400);

    table->setColumnCount(5);
    table->setHorizontalHeaderLabels({"Item", "Amount", "Equal split", "Paid for", "Paid by"});
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->horizontalHeader()->setStretchLastSection(true);

    amountEdit->setRange(0.0, 10000000.0);
    amountEdit->setDecimals(2);
    splitCheck->setChecked(true);

    QFormLayout* formLayout = new QFormLayout();
    formLayout->addRow("Item", itemEdit);
    formLayout->addRow("Amount", amountEdit);
    formLayout->addRow("Paid for", paidForCombo);
    formLayout->addRow("Paid by", paidByCombo);
    formLayout->addRow("", splitCheck);

    QHBoxLayout* buttonsLayout = new QHBoxLayout();
    buttonsLayout->addWidget(addButton);
    buttonsLayout->addWidget(removeButton);
    buttonsLayout->addWidget(closeButton);

    QVBoxLayout* dialogLayout = new QVBoxLayout(this);
    dialogLayout->addWidget(table);
    dialogLayout->addLayout(formLayout);
    dialogLayout->addLayout(buttonsLayout);

    connect(table, &QTableWidget::cellClicked, this, &RecurringExpensesDialog::onUserRowSelected);
    connect(addButton, &QPushButton::clicked, this, &RecurringExpensesDialog::addRecurringExpense);
    connect(removeButton, &QPushButton::clicked, this, &RecurringExpensesDialog::removeRecurringExpense);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    populateUserCombos();
    refreshTable();
}

void RecurringExpensesDialog::populateUserCombos() {
    paidForCombo->clear();
    paidByCombo->clear();
    paidForCombo->addItem("Both");
    for (int i = 1; i <= m_userList.size(); ++i) {
        const QString name = QString::fromStdString(m_userList.getUser(i).getName());
        paidForCombo->addItem(name);
        paidByCombo->addItem(name);
    }
}

void RecurringExpensesDialog::refreshTable() {
    table->setRowCount(m_recurringExpenses.size());
    for (int i = 1; i <= m_recurringExpenses.size(); ++i) {
        const Expense e = m_recurringExpenses.getExpense(i);
        table->setItem(i - 1, 0, new QTableWidgetItem(QString::fromStdString(e.getItem())));
        table->setItem(i - 1, 1, new QTableWidgetItem(QString::number(e.getAmount(), 'f', 2)));
        table->setItem(i - 1, 2, new QTableWidgetItem(e.isEqualSplit() ? "Yes" : "No"));
        table->setItem(i - 1, 3, new QTableWidgetItem(QString::fromStdString(e.getPaidFor().empty() ? "Both" : e.getPaidFor())));
        table->setItem(i - 1, 4, new QTableWidgetItem(QString::fromStdString(e.getPaidBy().getName().empty() ? "Unknown" : e.getPaidBy().getName())));
    }
}

void RecurringExpensesDialog::clearForm() {
    itemEdit->clear();
    amountEdit->setValue(0.0);
    splitCheck->setChecked(true);
    paidForCombo->setCurrentIndex(0);
    paidByCombo->setCurrentIndex(0);
}

void RecurringExpensesDialog::onUserRowSelected(int row, int) {
    if (row < 0) {
        return;
    }
    const Expense e = m_recurringExpenses.getExpense(row + 1);
    itemEdit->setText(QString::fromStdString(e.getItem()));
    amountEdit->setValue(e.getAmount());
    splitCheck->setChecked(e.isEqualSplit());
    const int paidForIndex = paidForCombo->findText(QString::fromStdString(e.getPaidFor().empty() ? "Both" : e.getPaidFor()));
    paidForCombo->setCurrentIndex(qMax(0, paidForIndex));
    const int paidByIndex = paidByCombo->findText(QString::fromStdString(e.getPaidBy().getName()));
    paidByCombo->setCurrentIndex(qMax(0, paidByIndex));
    table->selectRow(row);
}

void RecurringExpensesDialog::addRecurringExpense() {
    if (itemEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Missing data", "Please enter an item name.");
        return;
    }

    Expense e;
    e.setItem(itemEdit->text().trimmed().toStdString());
    e.setAmount(amountEdit->value());
    e.setEqualSplit(splitCheck->isChecked());
    e.setPaidFor(paidForCombo->currentText().toStdString());

    const QString payerName = paidByCombo->currentText();
    for (int j = 1; j <= m_userList.size(); ++j) {
        const User candidate = m_userList.getUser(j);
        if (candidate.getName() == payerName.toStdString()) {
            e.setPaidBy(candidate);
            break;
        }
    }

    m_recurringExpenses.addExpense(e);
    refreshTable();
    clearForm();
    emit recurringExpensesChanged();
}

void RecurringExpensesDialog::removeRecurringExpense() {
    if (table->currentRow() < 0) {
        QMessageBox::warning(this, "Selection needed", "Select a recurring expense first.");
        return;
    }

    m_recurringExpenses.deleteExpense(table->currentRow() + 1);
    refreshTable();
    clearForm();
    emit recurringExpensesChanged();
}
