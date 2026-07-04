#include "mainwindow.h"
#include <QMessageBox>
#include <QStringList>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include <QMenuBar>
#include <QHeaderView>
#include <QStringConverter>
#include <QStandardPaths>
#include <QInputDialog>
#include <QDate>
#include <QKeyEvent>
#include <QApplication>
#include <QDir>
#include <QtGlobal>


// Main window setup
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      userListWidget(nullptr),
      expenseListWidget(nullptr),
      resultArea(nullptr),
      userNameEdit(nullptr),
      userSalaryEdit(nullptr),
      addUserButton(nullptr),
      removeUserButton(nullptr),
      expenseItemEdit(nullptr),
      expenseAmountEdit(nullptr),
      expensePayerCombo(nullptr),
      expensePaidForCombo(nullptr),
      equalSplitCheck(nullptr),
      addExpenseButton(nullptr),
      removeExpenseButton(nullptr),
      monthSlider(nullptr),
      yearSpinBox(nullptr),
      monthFilterLabel(nullptr),
      selectedFilterMonth(1),
      selectedFilterYear(QDate::currentDate().year()) {
    QWidget* central = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(central);


    QGroupBox* monthFilterGroup = new QGroupBox("Expense month filter", this);
    QHBoxLayout* monthFilterLayout = new QHBoxLayout(monthFilterGroup);
    monthFilterLabel = new QLabel("Showing January 2026", this);
    monthSlider = new QSlider(Qt::Horizontal, this);
    monthSlider->setRange(1, 12);
    monthSlider->setValue(selectedFilterMonth);
    monthSlider->setTickPosition(QSlider::TicksBelow);
    monthSlider->setTickInterval(1);
    yearSpinBox = new QSpinBox(this);
    yearSpinBox->setRange(2000, 2100);
    yearSpinBox->setValue(selectedFilterYear);
    monthFilterLayout->addWidget(new QLabel("Month", this));
    monthFilterLayout->addWidget(monthSlider);
    monthFilterLayout->addWidget(new QLabel("Year", this));
    monthFilterLayout->addWidget(yearSpinBox);
    monthFilterLayout->addWidget(monthFilterLabel);
    monthFilterLayout->addStretch();
    connect(monthSlider, &QSlider::valueChanged, this, &MainWindow::updateExpenseViewFilter);
    connect(yearSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::updateExpenseViewFilter);

    QGroupBox* expensesGroup = new QGroupBox("Expense editor", this);
    QVBoxLayout* expensesLayout = new QVBoxLayout(expensesGroup);


    // Box 2: expense list
    expenseListWidget = new QTableWidget(this);
    expenseListWidget->setColumnCount(6);
    expenseListWidget->setHorizontalHeaderLabels({"Date", "Expense", "Amount", "Paid by", "Paid for", "Equal split"});
    expenseListWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    expenseListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    expenseListWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    expenseListWidget->setTabKeyNavigation(false);
    expenseListWidget->setFocusPolicy(Qt::StrongFocus);
    expenseListWidget->setAlternatingRowColors(true);
    expenseListWidget->horizontalHeader()->setStretchLastSection(true);
    connect(expenseListWidget, &QTableWidget::cellClicked, this, [this](int row, int column) {
        Q_UNUSED(column);
        if (row >= 0) {
            if (!(QApplication::keyboardModifiers() & Qt::ControlModifier) && !(QApplication::keyboardModifiers() & Qt::ShiftModifier)) {
                expenseListWidget->selectRow(row);
            }
            loadSelectedExpense();
        }
    });
    expenseListWidget->installEventFilter(this);
    expensesLayout->addWidget(expenseListWidget);

    QFormLayout* expenseFormLayout = new QFormLayout();
    expenseItemEdit = new QLineEdit(this);
    expenseAmountEdit = new QDoubleSpinBox(this);
    expenseAmountEdit->setRange(0.0, 10000000.0);
    expenseAmountEdit->setDecimals(2);
    expensePayerCombo = new QComboBox(this);
    expensePaidForCombo = new QComboBox(this);
    equalSplitCheck = new QCheckBox("Equal split", this);
    expenseFormLayout->addRow("Item", expenseItemEdit);
    expenseFormLayout->addRow("Amount", expenseAmountEdit);
    expenseFormLayout->addRow("Paid by", expensePayerCombo);
    expenseFormLayout->addRow("Paid for", expensePaidForCombo);
    expenseFormLayout->addRow("", equalSplitCheck);
    expensesLayout->addLayout(expenseFormLayout);

    QHBoxLayout* expenseButtonsLayout = new QHBoxLayout();
    addExpenseButton = new QPushButton("Add expense", this);
    removeExpenseButton = new QPushButton("Remove expense", this);
    connect(addExpenseButton, &QPushButton::clicked, this, &MainWindow::addExpense);
    connect(removeExpenseButton, &QPushButton::clicked, this, &MainWindow::removeExpense);
    expenseButtonsLayout->addWidget(addExpenseButton);
    expenseButtonsLayout->addWidget(removeExpenseButton);
    expensesLayout->addLayout(expenseButtonsLayout);

    // Box 3: Settlement result
    QGroupBox* splitGroup = new QGroupBox("Settlement result", this);
    QVBoxLayout* splitLayout = new QVBoxLayout(splitGroup);
    resultArea = new QTextEdit(this);
    resultArea->setReadOnly(true);
    resultArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    resultArea->setFixedHeight(resultArea->fontMetrics().lineSpacing() + 8);
    splitLayout->addWidget(resultArea);

    expensesGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    mainLayout->addWidget(monthFilterGroup);
    mainLayout->addWidget(expensesGroup, 1);
    mainLayout->addWidget(splitGroup);

    QMenuBar* menuBar = this->menuBar();
    QMenu* fileMenu = menuBar->addMenu("File");
    QAction* saveAction = fileMenu->addAction("Save results");
    QAction* loadAction = fileMenu->addAction("Load results");
    QAction* importAction = fileMenu->addAction("Import expenses...");
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveStateToFile);
    connect(loadAction, &QAction::triggered, this, &MainWindow::loadStateFromFile);
    connect(importAction, &QAction::triggered, this, &MainWindow::importExpensesFromFile);

    QMenu* settingsMenu = menuBar->addMenu("Settings");
    QAction* usersSettingsAction = settingsMenu->addAction("Users...");
    connect(usersSettingsAction, &QAction::triggered, this, &MainWindow::openUserSettings);

    setCentralWidget(central);
    setMinimumSize(900, 600);
    showMaximized();
    loadUserSettingsFromDisk();
    refreshUserList();
    loadExpenseSettingsFromDisk();
    updateExpenseViewFilter();
}


bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
    if (watched == expenseListWidget && event->type() == QEvent::KeyPress) {
        const auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->modifiers() == Qt::ControlModifier && keyEvent->key() == Qt::Key_A) {
            expenseListWidget->selectAll();
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

QString MainWindow::getAppDataDirectoryPath() const {
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dataDir.isEmpty()) {
        return QString();
    }
    QDir dir(dataDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return dir.absolutePath();
}

QString MainWindow::getUserSettingsFilePath() const {
    const QString dataDir = getAppDataDirectoryPath();
    return dataDir.isEmpty() ? QString() : QDir(dataDir).filePath("users.txt");
}

QString MainWindow::getExpenseSettingsFilePath() const {
    const QString dataDir = getAppDataDirectoryPath();
    return dataDir.isEmpty() ? QString() : QDir(dataDir).filePath("expenses.txt");
}

void MainWindow::saveUserSettingsToDisk() {
    const QString filePath = getUserSettingsFilePath();
    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    QTextStream out(&file);
    out << userList.size() << "\n";
    for (int i = 1; i <= userList.size(); ++i) {
        const User user = userList.getUser(i);
        out << QString::fromStdString(user.getName()) << "|"
            << user.getSalary() << "|"
            << QString::fromStdString(user.getCardNumber()) << "\n";
    }
    file.close();
}

void MainWindow::loadUserSettingsFromDisk() {
    const QString filePath = getUserSettingsFilePath();
    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextStream in(&file);
    const QString countLine = in.readLine();
    bool ok = false;
    const int userCount = countLine.toInt(&ok);
    if (!ok) {
        file.close();
        return;
    }

    userList.clearUsers();
    for (int i = 0; i < userCount; ++i) {
        const QString line = in.readLine();
        if (line.isEmpty()) {
            continue;
        }
        const QStringList parts = line.split('|');
        if (parts.size() >= 2) {
            User user;
            user.setName(parts[0].toStdString());
            user.setSalary(parts[1].toInt());
            if (parts.size() >= 3) {
                user.setCardNumber(parts[2].toStdString());
            }
            userList.addUser(user);
        }
    }
    file.close();
}

void MainWindow::saveExpenseSettingsToDisk() {
    const QString filePath = getExpenseSettingsFilePath();
    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    QTextStream out(&file);
    out << expenseList.size() << "\n";
    for (int i = 1; i <= expenseList.size(); ++i) {
        const Expense expense = expenseList.getExpense(i);
        out << QString::fromStdString(expense.getItem()) << "|"
            << expense.getAmount() << "|"
            << QString::fromStdString(expense.getPaidBy().getName()) << "|"
            << (expense.isEqualSplit() ? 1 : 0) << "|"
            << QString::fromStdString(expense.getDate()) << "|"
            << QString::fromStdString(expense.getStatementMonth()) << "|"
            << QString::fromStdString(expense.getPaidFor().empty() ? "Both" : expense.getPaidFor()) << "\n";
    }
    file.close();
}

void MainWindow::loadExpenseSettingsFromDisk() {
    const QString filePath = getExpenseSettingsFilePath();
    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        expenseList.clearExpenses();
        return;
    }

    expenseList.clearExpenses();
    QTextStream in(&file);
    bool ok = false;
    const int expenseCount = in.readLine().toInt(&ok);
    if (!ok) {
        file.close();
        return;
    }

    for (int i = 0; i < expenseCount; ++i) {
        const QString line = in.readLine();
        if (line.isEmpty()) {
            continue;
        }

        const QStringList parts = line.split('|');
        if (parts.size() < 4) {
            continue;
        }

        Expense expense;
        expense.setItem(parts[0].toStdString());
        expense.setAmount(parts[1].toDouble());
        expense.setEqualSplit(parts[3] == "1");

        const QString dateText = parts.value(4).trimmed();
        const QString statementMonthText = parts.value(5).trimmed();
        const QString paidForText = parts.value(6).trimmed();
        expense.setDate(dateText.isEmpty() ? QDate::currentDate().toString(Qt::ISODate).toStdString() : dateText.toStdString());
        expense.setStatementMonth(statementMonthText.isEmpty() ? QString("%1-%2").arg(selectedFilterYear, 4, 10, QLatin1Char('0')).arg(selectedFilterMonth, 2, 10, QLatin1Char('0')).toStdString() : statementMonthText.toStdString());
        expense.setPaidFor(paidForText.isEmpty() ? "Both" : paidForText.toStdString());

        User payer("Unknown", 0);
        const QString payerName = parts.value(2).trimmed();
        for (int j = 1; j <= userList.size(); ++j) {
            const User candidate = userList.getUser(j);
            if (candidate.getName() == payerName.toStdString()) {
                payer = candidate;
                break;
            }
        }
        expense.setPaidBy(payer);
        expenseList.addExpense(expense);
    }

    file.close();
}

void MainWindow::importExpensesFromFile() {
    const QString filePath = QFileDialog::getOpenFileName(this, "Import expenses", QString(), "Text files (*.txt *.csv);;All files (*.*)");
    if (filePath.isEmpty()) {
        return;
    }

    if (yearSpinBox) {
        selectedFilterYear = yearSpinBox->value();
    }
    if (monthSlider) {
        selectedFilterMonth = monthSlider->value();
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Import failed", "Unable to open the selected file.");
        return;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    int importedCount = 0;

    while (!in.atEnd()) {
        const QString line = in.readLine();
        if (line.trimmed().isEmpty()) {
            continue;
        }

        QStringList values;
        QString current;
        bool inQuotes = false;
        for (int i = 0; i < line.size(); ++i) {
            const QChar ch = line.at(i);
            if (ch == '"') {
                const bool isEscapedQuote = i + 1 < line.size() && line.at(i + 1) == '"';
                if (isEscapedQuote) {
                    current += '"';
                    ++i;
                } else {
                    inQuotes = !inQuotes;
                }
            } else if (ch == ',' && !inQuotes) {
                values << current;
                current.clear();
            } else {
                current += ch;
            }
        }
        values << current;

        if (values.size() < 13) {
            continue;
        }

        QString cardIdentifier = values.value(0).trimmed();
        QString date = values.value(3).trimmed();
        const QString item = values.value(5).trimmed();
        const QString amountField = values.value(11).trimmed();
        const QString refundField = values.value(12).trimmed();

        QString digitsOnly;
        for (const QChar& ch : cardIdentifier) {
            if (ch.isDigit()) {
                digitsOnly.append(ch);
            }
        }
        if (digitsOnly.size() > 4) {
            digitsOnly = digitsOnly.right(4);
        }
        cardIdentifier = digitsOnly;

        if (cardIdentifier.isEmpty() || item.isEmpty() || date.isEmpty()) {
            continue;
        }

        double amount = 0.0;
        bool amountParsed = false;
        if (!amountField.isEmpty()) {
            bool ok = false;
            amount = amountField.toDouble(&ok);
            if (ok) {
                amountParsed = true;
            }
        }
        if (!amountParsed && !refundField.isEmpty()) {
            bool ok = false;
            amount = refundField.toDouble(&ok);
            if (ok) {
                amount = -amount;
                amountParsed = true;
            }
        }

        if (!amountParsed) {
            continue;
        }

        User payer = userList.getUserByCardIdentifier(cardIdentifier.toStdString());
        if (payer.getName().empty() || payer.getCardIdentifier() != cardIdentifier.toStdString()) {
            payer = User(cardIdentifier.toStdString(), 0);
            payer.setCardNumber(cardIdentifier.toStdString());
            userList.addUser(payer);
        }

        Expense expense;
        expense.setItem(item.toStdString());
        expense.setAmount(amount);
        expense.setPaidBy(payer);
        expense.setCardIdentifier(cardIdentifier.toStdString());
        expense.setDate(date.toStdString());
        expense.setStatementMonth(QString("%1-%2").arg(selectedFilterYear, 4, 10, QLatin1Char('0')).arg(selectedFilterMonth, 2, 10, QLatin1Char('0')).toStdString());
        expense.setPaidFor("Both");
        expense.setEqualSplit(true);
        expenseList.addExpense(expense);
        ++importedCount;
    }

    file.close();
    refreshUserList();
    updateExpenseViewFilter();

    QMessageBox::information(this, "Import complete", QString("Imported %1 expenses.").arg(importedCount));
}

// Userlist methods
void MainWindow::refreshUserList() {
    if (userListWidget) {
        userListWidget->clear();
    }
    if (expensePayerCombo) {
        expensePayerCombo->clear();
    }
    if (expensePaidForCombo) {
        expensePaidForCombo->clear();
    }
    for (int i = 1; i <= userList.size(); ++i) {
        const User user = userList.getUser(i);
        if (userListWidget) {
            userListWidget->addItem(QString::fromStdString(user.getName() + " - " + std::to_string(user.getSalary()) + " - " + user.getCardNumber()));
        }
        if (expensePayerCombo) {
            expensePayerCombo->addItem(QString::fromStdString(user.getName()));
        }
        if (expensePaidForCombo) {
            expensePaidForCombo->addItem(QString::fromStdString(user.getName()));
        }
    }
    if (expensePaidForCombo) {
        expensePaidForCombo->insertItem(0, "Both");
        expensePaidForCombo->setCurrentIndex(0);
    }
}

void MainWindow::refreshExpenseList() {
    visibleExpenseIndices.clear();
    expenseListWidget->setRowCount(0);

    int visibleCount = 0;
    for (int i = 1; i <= expenseList.size(); ++i) {
        const Expense expense = expenseList.getExpense(i);
        if (!expenseMatchesCurrentMonthYear(expense)) {
            continue;
        }

        visibleExpenseIndices.append(i);
        expenseListWidget->setRowCount(visibleCount + 1);

        const std::string payerName = expense.getPaidBy().getName();
        const QString dateText = QString::fromStdString(expense.getDate());
        const QString amountText = QString::number(expense.getAmount(), 'f', 2);
        const QString equalSplitText = expense.isEqualSplit() ? "Yes" : "No";

        expenseListWidget->setItem(visibleCount, 0, new QTableWidgetItem(dateText));
        expenseListWidget->setItem(visibleCount, 1, new QTableWidgetItem(QString::fromStdString(expense.getItem())));
        expenseListWidget->setItem(visibleCount, 2, new QTableWidgetItem(amountText));
        expenseListWidget->setItem(visibleCount, 3, new QTableWidgetItem(QString::fromStdString(payerName.empty() ? "Unknown" : payerName)));
        expenseListWidget->setItem(visibleCount, 4, new QTableWidgetItem(QString::fromStdString(expense.getPaidFor().empty() ? "Both" : expense.getPaidFor())));
        expenseListWidget->setItem(visibleCount, 5, new QTableWidgetItem(equalSplitText));
        ++visibleCount;
    }

    if (resultArea) {
        computeSplit();
    }
}

void MainWindow::clearUserForm() {
    if (userNameEdit) {
        userNameEdit->clear();
    }
    if (userSalaryEdit) {
        userSalaryEdit->setValue(0);
    }
}

void MainWindow::clearExpenseForm() {
    expenseItemEdit->clear();
    expenseAmountEdit->setValue(0.0);
    equalSplitCheck->setChecked(true);
    if (expensePayerCombo->count() > 0) {
        expensePayerCombo->setCurrentIndex(0);
    }
    if (expensePaidForCombo) {
        const int bothIndex = expensePaidForCombo->findText("Both");
        expensePaidForCombo->setCurrentIndex(bothIndex >= 0 ? bothIndex : 0);
    }
}

void MainWindow::addUser() {
    if (!userNameEdit || !userSalaryEdit) {
        return;
    }
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
    if (!userListWidget || !userNameEdit || !userSalaryEdit) {
        return;
    }
    if (userListWidget->currentRow() < 0) {
        QMessageBox::warning(this, "Selection needed", "Select a user first.");
        return;
    }
    userList.deleteUser(userListWidget->currentRow() + 1);
    refreshUserList();
    clearUserForm();
}

void MainWindow::saveUser() {
    if (!userListWidget || !userNameEdit || !userSalaryEdit) {
        return;
    }
    if (userListWidget->currentRow() < 0) {
        QMessageBox::warning(this, "Selection needed", "Select a user first.");
        return;
    }
    const int index = userListWidget->currentRow() + 1;
    userList.modifyUser(index, userNameEdit->text().toStdString(), userSalaryEdit->value());
    refreshUserList();
}

void MainWindow::loadSelectedUser() {
    if (!userListWidget || !userNameEdit || !userSalaryEdit) {
        return;
    }
    if (userListWidget->currentRow() < 0) {
        return;
    }
    const User user = userList.getUser(userListWidget->currentRow() + 1);
    userNameEdit->setText(QString::fromStdString(user.getName()));
    userSalaryEdit->setValue(user.getSalary());
}


// Expense methods:
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
    expense.setDate(QDate::currentDate().toString(Qt::ISODate).toStdString());
    expense.setStatementMonth(QString("%1-%2").arg(selectedFilterYear, 4, 10, QLatin1Char('0')).arg(selectedFilterMonth, 2, 10, QLatin1Char('0')).toStdString());
    expense.setPaidFor(expensePaidForCombo->currentText().toStdString());
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
    const QList<QTableWidgetSelectionRange> selectedRanges = expenseListWidget->selectedRanges();
    if (selectedRanges.isEmpty()) {
        QMessageBox::warning(this, "Selection needed", "Select at least one expense first.");
        return;
    }

    QList<int> indicesToRemove;
    for (const QTableWidgetSelectionRange& range : selectedRanges) {
        for (int row = range.topRow(); row <= range.bottomRow(); ++row) {
            const int actualIndex = getSelectedExpenseIndexForRow(row);
            if (actualIndex > 0 && !indicesToRemove.contains(actualIndex)) {
                indicesToRemove.append(actualIndex);
            }
        }
    }

    std::sort(indicesToRemove.begin(), indicesToRemove.end(), std::greater<int>());
    for (const int index : indicesToRemove) {
        expenseList.deleteExpense(index);
    }

    refreshExpenseList();
    clearExpenseForm();
}

void MainWindow::saveExpense() {
    if (expenseListWidget->currentRow() < 0) {
        QMessageBox::warning(this, "Selection needed", "Select an expense first.");
        return;
    }
    const int index = getSelectedExpenseIndex();
    if (index <= 0) {
        return;
    }
    Expense expense;
    expense.setItem(expenseItemEdit->text().toStdString());
    expense.setAmount(expenseAmountEdit->value());
    const User payer = userList.getUser(expensePayerCombo->currentIndex() + 1);
    expense.setPaidBy(payer);
    expense.setPaidFor(expensePaidForCombo->currentText().toStdString());
    expense.setEqualSplit(equalSplitCheck->isChecked());
    expenseList.updateExpense(index, expense);
    refreshExpenseList();
}

void MainWindow::loadSelectedExpense() {
    if (expenseListWidget->currentRow() < 0) {
        return;
    }
    const int actualIndex = getSelectedExpenseIndex();
    if (actualIndex <= 0) {
        return;
    }
    const Expense expense = expenseList.getExpense(actualIndex);
    expenseItemEdit->setText(QString::fromStdString(expense.getItem()));
    expenseAmountEdit->setValue(expense.getAmount());
    equalSplitCheck->setChecked(expense.isEqualSplit());
    const int payerIndex = expensePayerCombo->findText(QString::fromStdString(expense.getPaidBy().getName()));
    if (payerIndex >= 0) {
        expensePayerCombo->setCurrentIndex(payerIndex);
    }
    const QString paidForValue = QString::fromStdString(expense.getPaidFor().empty() ? "Both" : expense.getPaidFor());
    const int paidForIndex = expensePaidForCombo->findText(paidForValue);
    if (paidForIndex >= 0) {
        expensePaidForCombo->setCurrentIndex(paidForIndex);
    }
}


void MainWindow::openUserSettings() {
    QDialog dialog(this);
    dialog.setWindowTitle("User settings");
    dialog.resize(400, 300);

    QVBoxLayout* dialogLayout = new QVBoxLayout(&dialog);

    QTableWidget* settingsUserList = new QTableWidget(&dialog);
    settingsUserList->setColumnCount(3);
    settingsUserList->setHorizontalHeaderLabels({"Name", "Salary", "Card number"});
    settingsUserList->setSelectionBehavior(QAbstractItemView::SelectRows);
    settingsUserList->setSelectionMode(QAbstractItemView::SingleSelection);
    settingsUserList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    settingsUserList->setAlternatingRowColors(true);
    settingsUserList->horizontalHeader()->setStretchLastSection(true);
    dialogLayout->addWidget(settingsUserList);

    QFormLayout* settingsFormLayout = new QFormLayout();
    QLineEdit* settingsNameEdit = new QLineEdit(&dialog);
    QSpinBox* settingsSalaryEdit = new QSpinBox(&dialog);
    QLineEdit* settingsCardNumberEdit = new QLineEdit(&dialog);
    settingsSalaryEdit->setRange(0, 10000000);
    settingsFormLayout->addRow("Name", settingsNameEdit);
    settingsFormLayout->addRow("Salary", settingsSalaryEdit);
    settingsFormLayout->addRow("Card number", settingsCardNumberEdit);
    dialogLayout->addLayout(settingsFormLayout);

    auto refreshSettingsUserList = [&]() {
        settingsUserList->setRowCount(0);
        settingsUserList->setRowCount(userList.size());
        for (int i = 1; i <= userList.size(); ++i) {
            const User user = userList.getUser(i);
            settingsUserList->setItem(i - 1, 0, new QTableWidgetItem(QString::fromStdString(user.getName())));
            settingsUserList->setItem(i - 1, 1, new QTableWidgetItem(QString::number(user.getSalary())));
            settingsUserList->setItem(i - 1, 2, new QTableWidgetItem(QString::fromStdString(user.getCardNumber())));
        }
    };

    connect(settingsUserList, &QTableWidget::cellClicked, &dialog, [settingsUserList, settingsNameEdit, settingsSalaryEdit, settingsCardNumberEdit, this](int row, int column) {
        Q_UNUSED(column);
        if (row < 0) {
            return;
        }
        const User user = userList.getUser(row + 1);
        settingsNameEdit->setText(QString::fromStdString(user.getName()));
        settingsSalaryEdit->setValue(user.getSalary());
        settingsCardNumberEdit->setText(QString::fromStdString(user.getCardNumber()));
        settingsUserList->selectRow(row);
    });

    QHBoxLayout* settingsButtonsLayout = new QHBoxLayout();
    QPushButton* addSettingsUserButton = new QPushButton("Add user", &dialog);
    QPushButton* removeSettingsUserButton = new QPushButton("Remove user", &dialog);
    QPushButton* closeSettingsButton = new QPushButton("Close", &dialog);

    connect(addSettingsUserButton, &QPushButton::clicked, &dialog, [this, settingsNameEdit, settingsSalaryEdit, settingsCardNumberEdit, settingsUserList, refreshSettingsUserList]() {
        const QString name = settingsNameEdit->text().trimmed();
        const QString cardNumber = settingsCardNumberEdit->text().trimmed();
        if (name.isEmpty()) {
            QMessageBox::warning(nullptr, "Missing data", "Please enter a user name.");
            return;
        }
        if (cardNumber.size() != 4 || !std::all_of(cardNumber.begin(), cardNumber.end(), [](QChar ch) { return ch.isDigit(); })) {
            QMessageBox::warning(nullptr, "Invalid input", "Please enter a 4-digit card number.");
            return;
        }
        User user;
        user.setName(name.toStdString());
        user.setSalary(settingsSalaryEdit->value());
        user.setCardNumber(cardNumber.toStdString());
        userList.addUser(user);
        saveUserSettingsToDisk();
        refreshSettingsUserList();
        refreshUserList();
        settingsNameEdit->clear();
        settingsSalaryEdit->setValue(0);
        settingsCardNumberEdit->clear();
    });

    connect(removeSettingsUserButton, &QPushButton::clicked, &dialog, [this, settingsUserList, settingsNameEdit, settingsSalaryEdit, settingsCardNumberEdit, refreshSettingsUserList]() {
        if (settingsUserList->currentRow() < 0) {
            QMessageBox::warning(nullptr, "Selection needed", "Select a user first.");
            return;
        }
        userList.deleteUser(settingsUserList->currentRow() + 1);
        saveUserSettingsToDisk();
        refreshSettingsUserList();
        refreshUserList();
        settingsNameEdit->clear();
        settingsSalaryEdit->setValue(0);
        settingsCardNumberEdit->clear();
    });

    connect(closeSettingsButton, &QPushButton::clicked, &dialog, &QDialog::accept);

    settingsButtonsLayout->addWidget(addSettingsUserButton);
    settingsButtonsLayout->addWidget(removeSettingsUserButton);
    settingsButtonsLayout->addWidget(closeSettingsButton);
    dialogLayout->addLayout(settingsButtonsLayout);

    refreshSettingsUserList();
    dialog.exec();
}

void MainWindow::saveStateToFile() {
    const QString filePath = getExpenseSettingsFilePath();
    if (filePath.isEmpty()) {
        return;
    }

    saveToFile(filePath);
    saveUserSettingsToDisk();
    saveExpenseSettingsToDisk();
    QMessageBox::information(this, "Saved", QString("Data were correctly saved to %1").arg(filePath));
}

void MainWindow::loadStateFromFile() {
    const QString filePath = QFileDialog::getOpenFileName(this, "Load results", QString(), "Expense app files (*.txt)");
    if (filePath.isEmpty()) {
        return;
    }
    loadFromFile(filePath);
}

void MainWindow::saveToFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Save failed", "Could not open the selected file for writing.");
        return;
    }

    QTextStream out(&file);
    out << "APP_DEPENSES_SAVE_V1\n";
    out << userList.size() << "\n";
    for (int i = 1; i <= userList.size(); ++i) {
        const User user = userList.getUser(i);
        out << QString::fromStdString(user.getName()) << "|" << user.getSalary() << "|" << QString::fromStdString(user.getCardNumber()) << "\n";
    }

    out << expenseList.size() << "\n";
    for (int i = 1; i <= expenseList.size(); ++i) {
        const Expense expense = expenseList.getExpense(i);
        out << QString::fromStdString(expense.getItem()) << "|"
            << expense.getAmount() << "|"
            << QString::fromStdString(expense.getPaidBy().getName()) << "|"
            << (expense.isEqualSplit() ? 1 : 0) << "|"
            << QString::fromStdString(expense.getDate()) << "|"
            << QString::fromStdString(expense.getStatementMonth()) << "|"
            << QString::fromStdString(expense.getPaidFor().empty() ? "" : expense.getPaidFor()) << "\n";
    }

    file.close();
}

void MainWindow::loadFromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Load failed", "Could not open the selected file.");
        return;
    }

    QTextStream in(&file);
    const QString header = in.readLine();
    if (header != "APP_DEPENSES_SAVE_V1") {
        file.close();
        QMessageBox::warning(this, "Load failed", "This file is not a valid saved result file.");
        return;
    }

    userList.clearUsers();
    expenseList.clearExpenses();

    int userCount = 0;
    bool ok = false;
    const QString userCountLine = in.readLine();
    userCount = userCountLine.toInt(&ok);
    if (!ok) {
        file.close();
        QMessageBox::warning(this, "Load failed", "The file contains invalid user data.");
        return;
    }

    for (int i = 0; i < userCount; ++i) {
        const QString line = in.readLine();
        if (line.isEmpty()) {
            continue;
        }
        const QStringList parts = line.split('|');
        if (parts.size() >= 2) {
            User user;
            user.setName(parts[0].toStdString());
            user.setSalary(parts[1].toInt());
            if (parts.size() >= 3) {
                user.setCardNumber(parts[2].toStdString());
            }
            userList.addUser(user);
        }
    }

    int expenseCount = 0;
    bool expenseOk = false;
    const QString expenseCountLine = in.readLine();
    expenseCount = expenseCountLine.toInt(&expenseOk);
    if (!expenseOk) {
        file.close();
        QMessageBox::warning(this, "Load failed", "The file contains invalid expense data.");
        return;
    }

    for (int i = 0; i < expenseCount; ++i) {
        const QString line = in.readLine();
        if (line.isEmpty()) {
            continue;
        }
        const QStringList parts = line.split('|');
        if (parts.size() >= 4) {
            Expense expense;
            expense.setItem(parts[0].toStdString());
            expense.setAmount(parts[1].toDouble());
            expense.setEqualSplit(parts[3] == "1");
            const QString dateText = parts.value(4).trimmed();
            const QString statementMonthText = parts.value(5).trimmed();
            const QString paidForText = parts.value(6).trimmed();
            expense.setDate(dateText.isEmpty() ? QDate::currentDate().toString(Qt::ISODate).toStdString() : dateText.toStdString());
            expense.setStatementMonth(statementMonthText.isEmpty() ? QString("%1-%2").arg(selectedFilterYear, 4, 10, QLatin1Char('0')).arg(selectedFilterMonth, 2, 10, QLatin1Char('0')).toStdString() : statementMonthText.toStdString());
            expense.setPaidFor(paidForText.toStdString());

            User payer("Unknown", 0);
            const QString payerName = parts.value(2).trimmed();
            for (int j = 1; j <= userList.size(); ++j) {
                const User candidate = userList.getUser(j);
                if (candidate.getName() == payerName.toStdString()) {
                    payer = candidate;
                    break;
                }
            }
            expense.setPaidBy(payer);
            expenseList.addExpense(expense);
        }
    }

    file.close();
    refreshUserList();
    updateExpenseViewFilter();
    clearUserForm();
    clearExpenseForm();
    resultArea->setPlainText("");
    QMessageBox::information(this, "Loaded", "The saved users and expenses were loaded successfully.");
}

void MainWindow::updateExpenseViewFilter() {
    if (monthSlider) {
        selectedFilterMonth = monthSlider->value();
    }
    if (yearSpinBox) {
        selectedFilterYear = yearSpinBox->value();
    }
    if (monthFilterLabel) {
        const QStringList monthNames = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
        monthFilterLabel->setText(QString("Showing %1 %2").arg(monthNames.value(selectedFilterMonth - 1, "Unknown")).arg(selectedFilterYear));
    }
    refreshExpenseList();
}

bool MainWindow::expenseMatchesCurrentMonthYear(const Expense& expense) const {
    const QString statementMonth = QString::fromStdString(expense.getStatementMonth()).trimmed();
    if (!statementMonth.isEmpty()) {
        const QStringList parts = statementMonth.split('-');
        if (parts.size() >= 2) {
            bool yearOk = false;
            bool monthOk = false;
            const int statementYear = parts[0].toInt(&yearOk);
            const int statementMonthValue = parts[1].toInt(&monthOk);
            if (yearOk && monthOk) {
                return statementYear == selectedFilterYear && statementMonthValue == selectedFilterMonth;
            }
        }
    }

    const QString dateText = QString::fromStdString(expense.getDate()).trimmed();
    if (dateText.isEmpty()) {
        return true;
    }

    QDate parsedDate = QDate::fromString(dateText, Qt::ISODate);
    if (!parsedDate.isValid()) {
        parsedDate = QDate::fromString(dateText, "yyyy/MM/dd");
    }
    if (!parsedDate.isValid()) {
        parsedDate = QDate::fromString(dateText, "dd/MM/yyyy");
    }
    if (!parsedDate.isValid()) {
        parsedDate = QDate::fromString(dateText, "yyyy-MM");
    }
    if (!parsedDate.isValid()) {
        return true;
    }

    return parsedDate.year() == selectedFilterYear && parsedDate.month() == selectedFilterMonth;
}

int MainWindow::getSelectedExpenseIndex() const {
    if (!expenseListWidget || expenseListWidget->currentRow() < 0) {
        return -1;
    }
    return getSelectedExpenseIndexForRow(expenseListWidget->currentRow());
}

int MainWindow::getSelectedExpenseIndexForRow(int row) const {
    if (!expenseListWidget || row < 0 || row >= visibleExpenseIndices.size()) {
        return -1;
    }
    return visibleExpenseIndices.at(row);
}

void MainWindow::computeSplit() {
    if (userList.size() < 2) {
        resultArea->setPlainText("Add at least two users to compute a split.");
        return;
    }

    userList.updateSalaryFactors();
    const User user1 = userList.getUser(1);
    const User user2 = userList.getUser(2);
    double balanceUser1 = 0.0;
    double balanceUser2 = 0.0;

    for (int i = 1; i <= expenseList.size(); ++i) {
        const Expense expense = expenseList.getExpense(i);
        if (!expenseMatchesCurrentMonthYear(expense)) {
            continue;
        }

        const QString paidForText = QString::fromStdString(expense.getPaidFor().empty() ? "Both" : expense.getPaidFor());
        const QString payerName = QString::fromStdString(expense.getPaidBy().getName());
        const double amount = expense.getAmount();

        if (paidForText == "Both") {
            if (payerName == QString::fromStdString(user1.getName())) {
                balanceUser1 += amount / 2.0;
                balanceUser2 -= amount / 2.0;
            } else if (payerName == QString::fromStdString(user2.getName())) {
                balanceUser2 += amount / 2.0;
                balanceUser1 -= amount / 2.0;
            }
        } else if (paidForText == QString::fromStdString(user1.getName())) {
            if (payerName == QString::fromStdString(user2.getName())) {
                balanceUser1 -= amount;
                balanceUser2 += amount;
            }
        } else if (paidForText == QString::fromStdString(user2.getName())) {
            if (payerName == QString::fromStdString(user1.getName())) {
                balanceUser1 += amount;
                balanceUser2 -= amount;
            }
        }
    }

    if (qAbs(balanceUser1) < 1e-9 && qAbs(balanceUser2) < 1e-9) {
        resultArea->setPlainText("Everyone is settled.");
        return;
    }

    QString result;
    if (balanceUser1 > 1e-9) {
        result += QString("%1 should receive %2 from %3.")
                      .arg(QString::fromStdString(user1.getName()))
                      .arg(QString::number(balanceUser1, 'f', 2))
                      .arg(QString::fromStdString(user2.getName()));
    } else if (balanceUser1 < -1e-9) {
        result += QString("%1 owes %2 to %3.")
                      .arg(QString::fromStdString(user1.getName()))
                      .arg(QString::number(-balanceUser1, 'f', 2))
                      .arg(QString::fromStdString(user2.getName()));
    }

    if (balanceUser2 > 1e-9) {
        if (!result.isEmpty()) {
            result += "\n";
        }
        result += QString("%1 should receive %2 from %3.")
                      .arg(QString::fromStdString(user2.getName()))
                      .arg(QString::number(balanceUser2, 'f', 2))
                      .arg(QString::fromStdString(user1.getName()));
    } else if (balanceUser2 < -1e-9) {
        if (!result.isEmpty()) {
            result += "\n";
        }
        result += QString("%1 owes %2 to %3.")
                      .arg(QString::fromStdString(user2.getName()))
                      .arg(QString::number(-balanceUser2, 'f', 2))
                      .arg(QString::fromStdString(user1.getName()));
    }

    resultArea->setPlainText(result.isEmpty() ? "Everyone is settled." : result);
}
