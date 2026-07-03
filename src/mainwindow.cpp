#include "mainwindow.h"
#include <QMessageBox>
#include <QStringList>
#include <QDebug>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    QWidget* central = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(central);

    QGroupBox* usersGroup = new QGroupBox("User list", this);
    QVBoxLayout* usersLayout = new QVBoxLayout(usersGroup);

    userListWidget = new QListWidget(this);
    connect(userListWidget, &QListWidget::currentRowChanged, this, [this](int row) {
        Q_UNUSED(row);
        loadSelectedUser();
    });
    usersLayout->addWidget(userListWidget);

    QFormLayout* userFormLayout = new QFormLayout();
    userNameEdit = new QLineEdit(this);
    userSalaryEdit = new QSpinBox(this);
    userSalaryEdit->setRange(0, 10000000);
    userFormLayout->addRow("Name", userNameEdit);
    userFormLayout->addRow("Salary", userSalaryEdit);
    usersLayout->addLayout(userFormLayout);

    QHBoxLayout* userButtonsLayout = new QHBoxLayout();
    addUserButton = new QPushButton("Add user", this);
    removeUserButton = new QPushButton("Remove user", this);
    saveUserButton = new QPushButton("Save user", this);
    connect(addUserButton, &QPushButton::clicked, this, &MainWindow::addUser);
    connect(removeUserButton, &QPushButton::clicked, this, &MainWindow::removeUser);
    connect(saveUserButton, &QPushButton::clicked, this, &MainWindow::saveUser);
    userButtonsLayout->addWidget(addUserButton);
    userButtonsLayout->addWidget(removeUserButton);
    userButtonsLayout->addWidget(saveUserButton);
    usersLayout->addLayout(userButtonsLayout);

    QGroupBox* expensesGroup = new QGroupBox("Expense editor", this);
    QVBoxLayout* expensesLayout = new QVBoxLayout(expensesGroup);

    expenseListWidget = new QListWidget(this);
    connect(expenseListWidget, &QListWidget::currentRowChanged, this, [this](int row) {
        Q_UNUSED(row);
        loadSelectedExpense();
    });
    expensesLayout->addWidget(expenseListWidget);

    QFormLayout* expenseFormLayout = new QFormLayout();
    expenseItemEdit = new QLineEdit(this);
    expenseAmountEdit = new QDoubleSpinBox(this);
    expenseAmountEdit->setRange(0.0, 10000000.0);
    expenseAmountEdit->setDecimals(2);
    expensePayerCombo = new QComboBox(this);
    equalSplitCheck = new QCheckBox("Equal split", this);
    expenseFormLayout->addRow("Item", expenseItemEdit);
    expenseFormLayout->addRow("Amount", expenseAmountEdit);
    expenseFormLayout->addRow("Paid by", expensePayerCombo);
    expenseFormLayout->addRow("", equalSplitCheck);
    expensesLayout->addLayout(expenseFormLayout);

    QHBoxLayout* expenseButtonsLayout = new QHBoxLayout();
    addExpenseButton = new QPushButton("Add expense", this);
    removeExpenseButton = new QPushButton("Remove expense", this);
    saveExpenseButton = new QPushButton("Save expense", this);
    connect(addExpenseButton, &QPushButton::clicked, this, &MainWindow::addExpense);
    connect(removeExpenseButton, &QPushButton::clicked, this, &MainWindow::removeExpense);
    connect(saveExpenseButton, &QPushButton::clicked, this, &MainWindow::saveExpense);
    expenseButtonsLayout->addWidget(addExpenseButton);
    expenseButtonsLayout->addWidget(removeExpenseButton);
    expenseButtonsLayout->addWidget(saveExpenseButton);
    expensesLayout->addLayout(expenseButtonsLayout);

    QGroupBox* splitGroup = new QGroupBox("Settlement result", this);
    QVBoxLayout* splitLayout = new QVBoxLayout(splitGroup);
    resultArea = new QTextEdit(this);
    resultArea->setReadOnly(true);
    splitLayout->addWidget(resultArea);
    computeButton = new QPushButton("Compute who owes what", this);
    connect(computeButton, &QPushButton::clicked, this, &MainWindow::computeSplit);
    splitLayout->addWidget(computeButton);

    mainLayout->addWidget(usersGroup);
    mainLayout->addWidget(expensesGroup);
    mainLayout->addWidget(splitGroup);

    setCentralWidget(central);
    refreshUserList();
    refreshExpenseList();
}

void MainWindow::refreshUserList() {
    userListWidget->clear();
    expensePayerCombo->clear();
    for (int i = 1; i <= userList.size(); ++i) {
        const User user = userList.getUser(i);
        userListWidget->addItem(QString::fromStdString(user.getName() + " - " + std::to_string(user.getSalary())));
        expensePayerCombo->addItem(QString::fromStdString(user.getName()));
    }
}

void MainWindow::refreshExpenseList() {
    expenseListWidget->clear();
    for (int i = 1; i <= expenseList.size(); ++i) {
        const Expense expense = expenseList.getExpense(i);
        expenseListWidget->addItem(QString::fromStdString(expense.getItem() + " - " + std::to_string(static_cast<int>(expense.getAmount()))));
    }
}

void MainWindow::clearUserForm() {
    userNameEdit->clear();
    userSalaryEdit->setValue(0);
}

void MainWindow::clearExpenseForm() {
    expenseItemEdit->clear();
    expenseAmountEdit->setValue(0.0);
    equalSplitCheck->setChecked(true);
    if (expensePayerCombo->count() > 0) {
        expensePayerCombo->setCurrentIndex(0);
    }
}

void MainWindow::addUser() {
    User user;
    user.setName(userNameEdit->text().toStdString());
    user.setSalary(userSalaryEdit->value());
    if (user.getName().empty()) {
        QMessageBox::warning(this, "Missing data", "Please enter a user name.");
        return;
    }
    userList.addUser(user);
    refreshUserList();
    clearUserForm();
}

void MainWindow::removeUser() {
    if (userListWidget->currentRow() < 0) {
        QMessageBox::warning(this, "Selection needed", "Select a user first.");
        return;
    }
    userList.deleteUser(userListWidget->currentRow() + 1);
    refreshUserList();
    clearUserForm();
}

void MainWindow::saveUser() {
    if (userListWidget->currentRow() < 0) {
        QMessageBox::warning(this, "Selection needed", "Select a user first.");
        return;
    }
    const int index = userListWidget->currentRow() + 1;
    userList.modifyUser(index, userNameEdit->text().toStdString(), userSalaryEdit->value());
    refreshUserList();
}

void MainWindow::loadSelectedUser() {
    if (userListWidget->currentRow() < 0) {
        return;
    }
    const User user = userList.getUser(userListWidget->currentRow() + 1);
    userNameEdit->setText(QString::fromStdString(user.getName()));
    userSalaryEdit->setValue(user.getSalary());
}

void MainWindow::addExpense() {
    if (expensePayerCombo->count() == 0) {
        QMessageBox::warning(this, "No user", "Add at least one user before creating an expense.");
        return;
    }
    Expense expense;
    expense.setItem(expenseItemEdit->text().toStdString());
    expense.setAmount(expenseAmountEdit->value());
    const User payer = userList.getUser(expensePayerCombo->currentIndex() + 1);
    expense.setPaidBy(payer);
    expense.setEqualSplit(equalSplitCheck->isChecked());
    if (expense.getItem().empty()) {
        QMessageBox::warning(this, "Missing data", "Please enter an expense item.");
        return;
    }
    expenseList.addExpense(expense);
    refreshExpenseList();
    clearExpenseForm();
}

void MainWindow::removeExpense() {
    if (expenseListWidget->currentRow() < 0) {
        QMessageBox::warning(this, "Selection needed", "Select an expense first.");
        return;
    }
    expenseList.deleteExpense(expenseListWidget->currentRow() + 1);
    refreshExpenseList();
    clearExpenseForm();
}

void MainWindow::saveExpense() {
    if (expenseListWidget->currentRow() < 0) {
        QMessageBox::warning(this, "Selection needed", "Select an expense first.");
        return;
    }
    const int index = expenseListWidget->currentRow() + 1;
    Expense expense;
    expense.setItem(expenseItemEdit->text().toStdString());
    expense.setAmount(expenseAmountEdit->value());
    const User payer = userList.getUser(expensePayerCombo->currentIndex() + 1);
    expense.setPaidBy(payer);
    expense.setEqualSplit(equalSplitCheck->isChecked());
    expenseList.updateExpense(index, expense);
    refreshExpenseList();
}

void MainWindow::loadSelectedExpense() {
    if (expenseListWidget->currentRow() < 0) {
        return;
    }
    const Expense expense = expenseList.getExpense(expenseListWidget->currentRow() + 1);
    expenseItemEdit->setText(QString::fromStdString(expense.getItem()));
    expenseAmountEdit->setValue(expense.getAmount());
    equalSplitCheck->setChecked(expense.isEqualSplit());
    const int payerIndex = expensePayerCombo->findText(QString::fromStdString(expense.getPaidBy().getName()));
    if (payerIndex >= 0) {
        expensePayerCombo->setCurrentIndex(payerIndex);
    }
}

void MainWindow::computeSplit() {
    if (userList.size() < 2) {
        resultArea->setPlainText("Add at least two users to compute a split.");
        return;
    }

    userList.updateSalaryFactors();
    double paidByUser1 = 0.0;
    double paidByUser2 = 0.0;

    for (int i = 1; i <= expenseList.size(); ++i) {
        const Expense expense = expenseList.getExpense(i);
        if (expense.getPaidBy().getName() == userList.getUser(1).getName()) {
            if (expense.isEqualSplit()) {
                paidByUser1 += expense.getAmount();
            } else {
                paidByUser1 += expense.getAmount() * userList.getUser(2).getSalaryFactor();
            }
        } else if (expense.getPaidBy().getName() == userList.getUser(2).getName()) {
            if (expense.isEqualSplit()) {
                paidByUser2 += expense.getAmount();
            } else {
                paidByUser2 += expense.getAmount() * userList.getUser(1).getSalaryFactor();
            }
        }
    }

    double totalPaid = paidByUser1 + paidByUser2;
    if (totalPaid == 0) {
        resultArea->setPlainText("No expenses to split.");
        return;
    }

    double splitAmount = totalPaid / 2.0;
    QString result;
    if (paidByUser1 > splitAmount) {
        result = QString::fromStdString(userList.getUser(1).getName() + " owes " + std::to_string(paidByUser1 - splitAmount));
    } else if (paidByUser2 > splitAmount) {
        result = QString::fromStdString(userList.getUser(2).getName() + " owes " + std::to_string(paidByUser2 - splitAmount));
    } else {
        result = "Everyone is settled.";
    }
    resultArea->setPlainText(result);
}
