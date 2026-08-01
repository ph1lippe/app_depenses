#include "mainwindow.h"
#include "settlement.h"
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
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QtGlobal>

class AmountTableWidgetItem : public QTableWidgetItem {
public:
    AmountTableWidgetItem(const QString& text, double value)
        : QTableWidgetItem(text), amountValue(value) {}

    bool operator<(const QTableWidgetItem& other) const override {
        if (other.type() == type()) {
            const auto* otherAmountItem = static_cast<const AmountTableWidgetItem*>(&other);
            return amountValue < otherAmountItem->amountValue;
        }
        return QTableWidgetItem::operator<(other);
    }

private:
    double amountValue;
};

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
    expenseListWidget->setColumnCount(7);
    expenseListWidget->setHorizontalHeaderLabels({"Date", "Equal split", "Amount", "Cardholder", "Paid by", "Paid for", "Item"});
    expenseListWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    expenseListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    expenseListWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    expenseListWidget->setTabKeyNavigation(false);
    expenseListWidget->setFocusPolicy(Qt::StrongFocus);
    expenseListWidget->setAlternatingRowColors(true);
    expenseListWidget->horizontalHeader()->setStretchLastSection(true);
    expenseListWidget->setSortingEnabled(true);
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
    connect(expenseListWidget->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this](const QItemSelection& selected, const QItemSelection&) {
        Q_UNUSED(selected);
        // update persistent selection index
        lastSelectedExpenseIndex = getSelectedExpenseIndex();
    });
    expensesLayout->addWidget(expenseListWidget);

    // Form layout to add individual expense:
    QFormLayout* expenseFormLayout = new QFormLayout();
    expenseItemEdit = new QLineEdit(this);
    expenseAmountEdit = new QDoubleSpinBox(this);
    expenseAmountEdit->setRange(0.0, 10000000.0);
    expenseAmountEdit->setDecimals(2);
    expenseAmountEdit->setSpecialValueText("");
    expenseDateEdit = new QLineEdit(this);
    expenseCardholderCombo = new QComboBox(this);
    expensePayerCombo = new QComboBox(this);
    expensePaidForCombo = new QComboBox(this);
    equalSplitCheck = new QCheckBox("Equal split", this);
    equalSplitCheck->setTristate(false);
    expenseFormLayout->addRow("Item", expenseItemEdit);
    expenseFormLayout->addRow("Amount", expenseAmountEdit);
    expenseFormLayout->addRow("Date", expenseDateEdit);
    expenseFormLayout->addRow("Cardholder", expenseCardholderCombo);
    expenseFormLayout->addRow("Paid by", expensePayerCombo);
    expenseFormLayout->addRow("Paid for", expensePaidForCombo);
    expenseFormLayout->addRow("", equalSplitCheck);
    expensesLayout->addLayout(expenseFormLayout);

    connect(expenseItemEdit, &QLineEdit::textEdited, this, [this]() { if (!m_updatingExpenseForm) saveExpense(ExpenseField::Item); });
    connect(expenseAmountEdit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this]() { if (!m_updatingExpenseForm) saveExpense(ExpenseField::Amount); });
    expenseDateEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("^\\d{4}-\\d{2}-\\d{2}$"), expenseDateEdit));
    connect(expenseDateEdit, &QLineEdit::editingFinished, this, [this]() { if (!m_updatingExpenseForm) saveExpense(ExpenseField::Date); });
    connect(expenseCardholderCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() { if (!m_updatingExpenseForm) saveExpense(ExpenseField::Cardholder); });
    connect(expensePayerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() { if (!m_updatingExpenseForm) saveExpense(ExpenseField::PaidBy); });
    connect(expensePaidForCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() { if (!m_updatingExpenseForm) saveExpense(ExpenseField::PaidFor); });
    connect(equalSplitCheck, &QCheckBox::toggled, this, [this]() { if (!m_updatingExpenseForm) saveExpense(ExpenseField::EqualSplit); });

    QHBoxLayout* expenseButtonsLayout = new QHBoxLayout();
    addExpenseButton = new QPushButton("New expense", this);
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
    QAction* recurringAction = settingsMenu->addAction("Recurring expenses...");
    connect(usersSettingsAction, &QAction::triggered, this, &MainWindow::openUserSettings);
    connect(recurringAction, &QAction::triggered, this, &MainWindow::openRecurringExpensesSettings);

    setCentralWidget(central);
    setMinimumSize(900, 600);
    showMaximized();
    loadUserSettingsFromDisk();
    refreshUserList();
    loadExpenseSettingsFromDisk();
    loadRecurringExpensesFromDisk();
    updateExpenseViewFilter();
}



// Event filter:
bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
    
    if (watched == expenseListWidget && event->type() == QEvent::KeyPress) {
        const auto* keyEvent = static_cast<QKeyEvent*>(event);
        // If selected month changes:
        if (keyEvent->modifiers() == Qt::ControlModifier && keyEvent->key() == Qt::Key_A) {
            expenseListWidget->selectAll();
            return true;
        }
        // Delete rows if "delete" pressed on keyboard:
        if (keyEvent->key() == Qt::Key_Delete){
            MainWindow::removeExpense();
        }
    }
    return QMainWindow::eventFilter(watched, event);
}


// Methods to save users and expenses to AppData
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
            << QString::fromStdString(expense.getPaidFor().empty() ? "Both" : expense.getPaidFor()) << "|"
            << QString::fromStdString(expense.getCardholder()) << "\n";
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

        // whenever expenses are loaded, statementMonth is attributed to expenses from the current month filter value:
        const QString dateText = parts.value(4).trimmed();
        const QString statementMonthText = parts.value(5).trimmed();
        const QString paidForText = parts.value(6).trimmed();
        const QString cardholderText = parts.value(7).trimmed();
        if (dateText.isEmpty()) {
            expense.setDate(QDate::currentDate().toString(Qt::ISODate).toStdString());
        } else {
            const QDate parsed = parseDate(dateText);
            if (parsed.isValid()) {
                expense.setDate(parsed.toString(Qt::ISODate).toStdString());
            } else {
                expense.setDate(QDate::currentDate().toString(Qt::ISODate).toStdString());
            }
        }
        expense.setStatementMonth(statementMonthText.isEmpty() ? QString("%1-%2").arg(selectedFilterYear, 4, 10, QLatin1Char('0')).arg(selectedFilterMonth, 2, 10, QLatin1Char('0')).toStdString() : statementMonthText.toStdString());
        expense.setPaidFor(paidForText.isEmpty() ? "Both" : paidForText.toStdString());
        expense.setCardholder(cardholderText.isEmpty() ? std::string() : cardholderText.toStdString());

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

    // If we loaded expenses, set the filter to the first expense's statement month
    if (expenseList.size() > 0) {
        const QString firstStatement = QString::fromStdString(expenseList.getExpense(1).getStatementMonth()).trimmed();
        if (!firstStatement.isEmpty()) {
            const QStringList parts = firstStatement.split('-');
            if (parts.size() >= 2) {
                bool yok = false, mok = false;
                const int y = parts[0].toInt(&yok);
                const int m = parts[1].toInt(&mok);
                if (yok && mok) {
                    selectedFilterYear = y;
                    selectedFilterMonth = m;
                    if (yearSpinBox) yearSpinBox->setValue(selectedFilterYear);
                    if (monthSlider) monthSlider->setValue(selectedFilterMonth);
                }
            }
        }
    }
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
    QDate firstImportedDate;

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

        // Normalize imported date to ISO format; skip if unparseable
        const QDate importedDate = parseDate(date);
        if (!importedDate.isValid()) {
            continue;
        }
        const QString isoDate = importedDate.toString(Qt::ISODate);

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

        if (!firstImportedDate.isValid()) {
            firstImportedDate = importedDate;
        }

        const User payer = userList.getUserByCardIdentifier(cardIdentifier.toStdString());
        const bool knownPayer = !payer.getName().empty() && payer.getCardIdentifier() == cardIdentifier.toStdString();

        Expense expense;
        expense.setItem(item.toStdString());
        expense.setAmount(amount);
        expense.setPaidBy(payer);
        expense.setCardholder(knownPayer ? payer.getName() : std::string());
        expense.setCardIdentifier(cardIdentifier.toStdString());
        expense.setDate(isoDate.toStdString());
        expense.setStatementMonth(QString("%1-%2").arg(importedDate.year(), 4, 10, QLatin1Char('0')).arg(importedDate.month(), 2, 10, QLatin1Char('0')).toStdString());
        expense.setPaidFor("Both");
        expense.setEqualSplit(true);
        expenseList.addExpense(expense);
        ++importedCount;
    }

    file.close();
    refreshUserList();

    if (firstImportedDate.isValid()) {
        selectedFilterYear = firstImportedDate.year();
        selectedFilterMonth = firstImportedDate.month();
        if (yearSpinBox) {
            yearSpinBox->setValue(selectedFilterYear);
        }
        if (monthSlider) {
            monthSlider->setValue(selectedFilterMonth);
        }
    }

    updateExpenseViewFilter();

    QMessageBox::information(this, "Import complete", QString("Imported %1 expenses.").arg(importedCount));
}

// Userlist methods
void MainWindow::refreshUserList() {
    if (userListWidget) {
        userListWidget->clear();
    }

    if (expensePayerCombo) {
        expensePayerCombo->blockSignals(true);
        expensePayerCombo->clear();
    }
    if (expenseCardholderCombo) {
        expenseCardholderCombo->blockSignals(true);
        expenseCardholderCombo->clear();
    }
    if (expensePaidForCombo) {
        expensePaidForCombo->blockSignals(true);
        expensePaidForCombo->clear();
        expensePaidForCombo->addItem("Both");
    }

    for (int i = 1; i <= userList.size(); ++i) {
        const User user = userList.getUser(i);
        if (userListWidget) {
            userListWidget->addItem(QString::fromStdString(user.getName() + " - " + std::to_string(user.getSalary()) + " - " + user.getCardNumber()));
        }
        if (expensePayerCombo) {
            expensePayerCombo->addItem(QString::fromStdString(user.getName()));
        }
        if (expenseCardholderCombo) {
            expenseCardholderCombo->addItem(QString::fromStdString(user.getName()));
        }
        if (expensePaidForCombo) {
            expensePaidForCombo->addItem(QString::fromStdString(user.getName()));
        }
    }

    if (expensePayerCombo) {
        expensePayerCombo->blockSignals(false);
        expensePayerCombo->setCurrentIndex(-1);
    }
    if (expenseCardholderCombo) {
        expenseCardholderCombo->blockSignals(false);
        expenseCardholderCombo->setCurrentIndex(-1);
    }
    if (expensePaidForCombo) {
        expensePaidForCombo->blockSignals(false);
        expensePaidForCombo->setCurrentIndex(0);
    }
}

// Update expense list:
void MainWindow::refreshExpenseList() {
    // Preserve currently selected expense (by expense index) across refreshes
    int previouslySelectedExpenseIndex = getSelectedExpenseIndex();
    
    if (previouslySelectedExpenseIndex <= 0 && lastSelectedExpenseIndex > 0) {
        previouslySelectedExpenseIndex = lastSelectedExpenseIndex;
    }

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

        QTableWidgetItem* dateItem = new QTableWidgetItem(dateText);
        dateItem->setData(Qt::UserRole, i);
        const QDate parsed = parseDate(dateText);
        if (parsed.isValid()) {
            dateItem->setData(Qt::EditRole, parsed);
        }
        expenseListWidget->setItem(visibleCount, 0, dateItem);
        expenseListWidget->setItem(visibleCount, 1, new QTableWidgetItem(equalSplitText));
        QTableWidgetItem* amountItem = new AmountTableWidgetItem(amountText, expense.getAmount());
        amountItem->setData(Qt::EditRole, expense.getAmount());
        expenseListWidget->setItem(visibleCount, 2, amountItem);
        const QString cardholderName = QString::fromStdString(expense.getCardholder());
        expenseListWidget->setItem(visibleCount, 3, new QTableWidgetItem(cardholderName));
        expenseListWidget->setItem(visibleCount, 4, new QTableWidgetItem(QString::fromStdString(payerName.empty() ? "Unknown" : payerName)));
        expenseListWidget->setItem(visibleCount, 5, new QTableWidgetItem(QString::fromStdString(expense.getPaidFor().empty() ? "Both" : expense.getPaidFor())));
        expenseListWidget->setItem(visibleCount, 6, new QTableWidgetItem(QString::fromStdString(expense.getItem())));
        ++visibleCount;
    }

    // Debug: show visible indices
    
    QString visibleStr;
    for (int v : visibleExpenseIndices) {
        visibleStr += QString::number(v) + ",";
    }
    

    // Try to restore previous selection if that expense is still visible
    if (previouslySelectedExpenseIndex > 0) {
        const int rowToSelect = visibleExpenseIndices.indexOf(previouslySelectedExpenseIndex);
        
        if (rowToSelect >= 0) {
            expenseListWidget->selectionModel()->select(
                expenseListWidget->model()->index(rowToSelect, 0),
                QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            expenseListWidget->setCurrentCell(rowToSelect, 0);
            lastSelectedExpenseIndex = previouslySelectedExpenseIndex;
            
        }
    } else {
    }

    // If nothing is selected but there are visible expenses, auto-select the first one
    const QModelIndexList selRows = expenseListWidget->selectionModel()->selectedRows(0);
    if (selRows.isEmpty() && !visibleExpenseIndices.isEmpty()) {
        expenseListWidget->selectionModel()->select(
            expenseListWidget->model()->index(0, 0),
            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        expenseListWidget->setCurrentCell(0, 0);
        // update persistent index
        lastSelectedExpenseIndex = visibleExpenseIndices.value(0);
        
        loadSelectedExpense();
    }

    if (resultArea) {
        computeSplit();
    }

    // Restore expense form state after rebuilding the list
    const QModelIndexList selectedRows = expenseListWidget->selectionModel()->selectedRows(0);
    if (selectedRows.size() == 1) {
        loadSelectedExpense();
    } else if (selectedRows.size() > 1) {
        clearExpenseFormForMultiSelection();
    } else {
        clearExpenseForm();
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
    m_updatingExpenseForm = true;
    expenseItemEdit->clear();
    expenseItemEdit->setPlaceholderText("");
    expenseDateEdit->clear();
    expenseDateEdit->setPlaceholderText("");
    if (expenseCardholderCombo) {
        expenseCardholderCombo->setCurrentIndex(-1);
    }
    expenseAmountEdit->setSpecialValueText("");
    expenseAmountEdit->setValue(0.0);
    equalSplitCheck->setCheckState(Qt::Unchecked);
    if (expensePayerCombo && expensePayerCombo->count() > 0) {
        expensePayerCombo->setCurrentIndex(-1);
    }
    if (expensePaidForCombo) {
        expensePaidForCombo->setCurrentIndex(-1);
    }
    m_updatingExpenseForm = false;
}

void MainWindow::clearExpenseFormForMultiSelection() {
    m_updatingExpenseForm = true;
    expenseItemEdit->clear();
    expenseItemEdit->setPlaceholderText("(multiple)");
    expenseDateEdit->clear();
    expenseDateEdit->setPlaceholderText("(multiple)");
    if (expenseCardholderCombo) {
        ensureComboPlaceholder(expenseCardholderCombo, "(multiple)");
        expenseCardholderCombo->setCurrentIndex(0);
    }
    expenseAmountEdit->setSpecialValueText("(multiple)");
    expenseAmountEdit->setValue(expenseAmountEdit->minimum());
    ensureComboPlaceholder(expensePayerCombo, "(multiple)");
    ensureComboPlaceholder(expensePaidForCombo, "(multiple)");
    if (expensePayerCombo) {
        expensePayerCombo->setCurrentIndex(0);
    }
    if (expensePaidForCombo) {
        expensePaidForCombo->setCurrentIndex(0);
    }
    equalSplitCheck->setCheckState(Qt::Unchecked);
    m_updatingExpenseForm = false;
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
    if (expensePayerCombo->count() <= 1) {
        QMessageBox::warning(this, "No user", "Add at least one user before creating an expense.");
        return;
    }
    Expense expense;
    expense.setItem("");
    expense.setAmount(0.0);
    expense.setPaidFor("Both");
    expense.setEqualSplit(false);
    expense.setDate(QDate::currentDate().toString(Qt::ISODate).toStdString());
    expense.setStatementMonth(QString("%1-%2").arg(selectedFilterYear, 4, 10, QLatin1Char('0')).arg(selectedFilterMonth, 2, 10, QLatin1Char('0')).toStdString());
    expenseList.addExpense(expense);
    refreshExpenseList();
    if (expenseListWidget->rowCount() > 0) {
        const int lastRow = expenseListWidget->rowCount() - 1;
        expenseListWidget->selectionModel()->select(
            expenseListWidget->model()->index(lastRow, 0),
            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        expenseListWidget->setCurrentCell(lastRow, 0);
    }
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

void MainWindow::saveExpense(ExpenseField field) {
    const QModelIndexList selectedRows = expenseListWidget->selectionModel()->selectedRows(0);
    if (selectedRows.isEmpty()) {
        return;
    }
    const User payer = (expensePayerCombo->currentIndex() >= 0)
        ? userList.getUser(expensePayerCombo->currentIndex() + 1)
        : User("Unknown", 0);

    const QString amountText = QString::number(expenseAmountEdit->value(), 'f', 2);
    const QString dateText = expenseDateEdit->text();
    QString cardholderText = expenseCardholderCombo->currentIndex() >= 0 ? expenseCardholderCombo->currentText() : QString();
    if (cardholderText == "(multiple)") {
        cardholderText.clear();
    }
    const QString payerText = expensePayerCombo->currentText();
    const QString paidForText = expensePaidForCombo->currentText();
    const QString itemText = expenseItemEdit->text();
    const QString equalSplitText = equalSplitCheck->isChecked() ? "Yes" : "No";

    if (field == ExpenseField::Date || field == ExpenseField::All) {
        if (!isIsoDateValid(dateText)) {
            QMessageBox::warning(this, "Invalid date", "Please enter a valid ISO date in the form YYYY-MM-DD.");
            return;
        }
    }

    for (const QModelIndex& selectedIndex : selectedRows) {
        const int row = selectedIndex.row();
        const int index = getSelectedExpenseIndexForRow(row);
        if (index <= 0) {
            continue;
        }
        Expense expense = expenseList.getExpense(index);
        switch (field) {
            case ExpenseField::Item:
                expense.setItem(itemText.toStdString());
                expenseListWidget->item(row, 6)->setText(itemText);
                break;
            case ExpenseField::Amount:
                expense.setAmount(expenseAmountEdit->value());
                expenseListWidget->item(row, 2)->setText(amountText);
                break;
            case ExpenseField::Date:
                expense.setDate(dateText.toStdString());
                expenseListWidget->item(row, 0)->setText(dateText);
                break;
            case ExpenseField::Cardholder:
                expense.setCardholder(cardholderText == "(multiple)" ? std::string() : cardholderText.toStdString());
                expenseListWidget->item(row, 3)->setText(cardholderText == "(multiple)" ? QString() : cardholderText);
                break;
            case ExpenseField::PaidBy:
                if (expensePayerCombo->currentIndex() >= 0) {
                    expense.setPaidBy(payer);
                    expenseListWidget->item(row, 4)->setText(payerText);
                }
                break;
            case ExpenseField::PaidFor:
                expense.setPaidFor(paidForText.toStdString());
                expenseListWidget->item(row, 5)->setText(paidForText);
                break;
            case ExpenseField::EqualSplit:
                expense.setEqualSplit(equalSplitCheck->isChecked());
                expenseListWidget->item(row, 1)->setText(equalSplitText);
                break;
            case ExpenseField::All:
                expense.setItem(itemText.toStdString());
                expense.setAmount(expenseAmountEdit->value());
                expense.setDate(dateText.toStdString());
                expense.setCardholder(cardholderText == "(multiple)" ? std::string() : cardholderText.toStdString());
                expense.setEqualSplit(equalSplitCheck->isChecked());
                expense.setPaidFor(paidForText.toStdString());
                if (expensePayerCombo->currentIndex() >= 0) {
                    expense.setPaidBy(payer);
                }
                expenseListWidget->item(row, 6)->setText(itemText);
                expenseListWidget->item(row, 2)->setText(amountText);
                expenseListWidget->item(row, 0)->setText(dateText);
                expenseListWidget->item(row, 3)->setText(cardholderText);
                expenseListWidget->item(row, 1)->setText(equalSplitText);
                expenseListWidget->item(row, 4)->setText(payerText);
                expenseListWidget->item(row, 5)->setText(paidForText);
                break;
        }
        expenseList.updateExpense(index, expense);
    }

    // Update settlement result immediately after changes
    if (resultArea) {
        computeSplit();
    }

    saveExpenseSettingsToDisk();
}

bool MainWindow::isIsoDateValid(const QString& dateText) const {
    const QDate parsedDate = QDate::fromString(dateText, Qt::ISODate);
    return parsedDate.isValid() && QRegularExpression("^\\d{4}-\\d{2}-\\d{2}$").match(dateText).hasMatch();
}

QDate MainWindow::parseDate(const QString& dateText) const {
    if (dateText.isEmpty()) return QDate();
    QDate d = QDate::fromString(dateText, Qt::ISODate);
    if (d.isValid()) return d;
    // Common alternative formats
    d = QDate::fromString(dateText, "dd/MM/yyyy");
    if (d.isValid()) return d;
    d = QDate::fromString(dateText, "MM/dd/yyyy");
    if (d.isValid()) return d;
    d = QDate::fromString(dateText, "dd.MM.yyyy");
    if (d.isValid()) return d;
    d = QDate::fromString(dateText, "yyyy-MM");
    if (d.isValid()) return d;
    // Fallback: Qt's default parsing
    d = QDate::fromString(dateText);
    return d;
}

void MainWindow::ensureComboPlaceholder(QComboBox* combo, const QString& placeholder) {
    if (!combo) {
        return;
    }
    if (combo->findText(placeholder) < 0) {
        combo->insertItem(0, placeholder);
    }
}

void MainWindow::removeComboPlaceholder(QComboBox* combo, const QString& placeholder) {
    if (!combo) {
        return;
    }
    const int index = combo->findText(placeholder);
    if (index >= 0) {
        combo->removeItem(index);
    }
}

void MainWindow::loadSelectedExpense() {
    const QModelIndexList selectedRows = expenseListWidget->selectionModel()->selectedRows(0);
    if (selectedRows.size() != 1) {
        clearExpenseFormForMultiSelection();
        return;
    }
    removeComboPlaceholder(expenseCardholderCombo, "(multiple)");
    removeComboPlaceholder(expensePayerCombo, "(multiple)");
    removeComboPlaceholder(expensePaidForCombo, "(multiple)");
    const int actualIndex = getSelectedExpenseIndex();
    if (actualIndex <= 0) {
        return;
    }
    // remember last selected expense index so selection can be restored across refreshes/filters
    lastSelectedExpenseIndex = actualIndex;
    const Expense expense = expenseList.getExpense(actualIndex);
    m_updatingExpenseForm = true;
    expenseItemEdit->setText(QString::fromStdString(expense.getItem()));
    expenseItemEdit->setPlaceholderText("");
    expenseDateEdit->setText(QString::fromStdString(expense.getDate()));
    expenseDateEdit->setPlaceholderText("");
    const QString cardholderName = QString::fromStdString(expense.getCardholder());
    const int cardholderIndex = expenseCardholderCombo->findText(cardholderName);
    if (cardholderIndex >= 0) {
        expenseCardholderCombo->setCurrentIndex(cardholderIndex);
    } else {
        expenseCardholderCombo->setCurrentIndex(-1);
    }
    expenseAmountEdit->setSpecialValueText("");
    expenseAmountEdit->setValue(expense.getAmount());
    equalSplitCheck->setCheckState(expense.isEqualSplit() ? Qt::Checked : Qt::Unchecked);
    const int payerIndex = expensePayerCombo->findText(QString::fromStdString(expense.getPaidBy().getName()));
    if (payerIndex >= 0) {
        expensePayerCombo->setCurrentIndex(payerIndex);
    } else {
        expensePayerCombo->setCurrentIndex(-1);
    }
    const QString paidForValue = QString::fromStdString(expense.getPaidFor().empty() ? "Both" : expense.getPaidFor());
    const int paidForIndex = expensePaidForCombo->findText(paidForValue);
    if (paidForIndex >= 0) {
        expensePaidForCombo->setCurrentIndex(paidForIndex);
    } else if (expensePaidForCombo->count() > 0) {
        expensePaidForCombo->setCurrentIndex(0);
    }
    m_updatingExpenseForm = false;
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
            << QString::fromStdString(expense.getPaidFor().empty() ? "" : expense.getPaidFor()) << "|"
            << QString::fromStdString(expense.getCardholder()) << "\n";
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
            const QString cardholderText = parts.value(7).trimmed();
            expense.setDate(dateText.isEmpty() ? QDate::currentDate().toString(Qt::ISODate).toStdString() : dateText.toStdString());
            expense.setStatementMonth(statementMonthText.isEmpty() ? QString("%1-%2").arg(selectedFilterYear, 4, 10, QLatin1Char('0')).arg(selectedFilterMonth, 2, 10, QLatin1Char('0')).toStdString() : statementMonthText.toStdString());
            expense.setPaidFor(paidForText.toStdString());
            expense.setCardholder(cardholderText.toStdString());

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
    applyRecurringExpensesIfNeeded();
    // Temporarily disable sorting when the month filter changes so the refresh uses natural order
    if (expenseListWidget) {
        const bool wasSorting = expenseListWidget->isSortingEnabled();
        if (wasSorting) {
            expenseListWidget->setSortingEnabled(false);
            if (expenseListWidget->horizontalHeader()) {
                expenseListWidget->horizontalHeader()->setSortIndicatorShown(false);
            }
        }

        refreshExpenseList();

        // Re-enable sorting so the user can sort again, but the previous sort is cleared by the refresh
        if (wasSorting) {
            expenseListWidget->setSortingEnabled(true);
            if (expenseListWidget->horizontalHeader()) {
                expenseListWidget->horizontalHeader()->setSortIndicatorShown(true);
            }
        }
        return;
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
    if (!expenseListWidget || row < 0 || row >= expenseListWidget->rowCount()) {
        return -1;
    }
    const QTableWidgetItem* item = expenseListWidget->item(row, 0);
    if (!item) {
        return -1;
    }
    return item->data(Qt::UserRole).toInt();
}

void MainWindow::computeSplit() {
    const std::string result = computeSettlementResult(userList, expenseList, selectedFilterYear, selectedFilterMonth);
    resultArea->setPlainText(QString::fromStdString(result));
}

// Recurring expenses:
QString MainWindow::getRecurringExpensesFilePath() const {
    const QString dataDir = getAppDataDirectoryPath();
    return dataDir.isEmpty() ? QString() : QDir(dataDir).filePath("recurring.txt");
}

void MainWindow::saveRecurringExpensesToDisk() {
    const QString filePath = getRecurringExpensesFilePath();
    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    QTextStream out(&file);
    out << "RECURRING_V1\n";
    out << recurringExpenses.size() << "\n";
    for (int i = 1; i <= recurringExpenses.size(); ++i) {
        const Expense e = recurringExpenses.getExpense(i);
        out << QString::fromStdString(e.getItem()) << "|"
            << e.getAmount() << "|"
            << (e.isEqualSplit() ? 1 : 0) << "|"
            << QString::fromStdString(e.getPaidFor().empty() ? "Both" : e.getPaidFor()) << "|"
            << QString::fromStdString(e.getPaidBy().getName()) << "\n";
    }
    file.close();
}

void MainWindow::loadRecurringExpensesFromDisk() {
    const QString filePath = getRecurringExpensesFilePath();
    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextStream in(&file);
    if (in.readLine() != "RECURRING_V1") {
        file.close();
        return;
    }

    recurringExpenses.clearExpenses();
    bool ok = false;
    const int count = in.readLine().toInt(&ok);
    if (!ok) {
        file.close();
        return;
    }

    for (int i = 0; i < count; ++i) {
        const QStringList parts = in.readLine().split('|');
        if (parts.size() < 4) {
            continue;
        }
        Expense e;
        e.setItem(parts[0].toStdString());
        e.setAmount(parts[1].toDouble());
        e.setEqualSplit(parts[2] == "1");
        e.setPaidFor(parts[3].toStdString());
        if (parts.size() >= 5) {
            const QString payerName = parts[4].trimmed();
            User payer("Unknown", 0);
            for (int j = 1; j <= userList.size(); ++j) {
                const User candidate = userList.getUser(j);
                if (candidate.getName() == payerName.toStdString()) {
                    payer = candidate;
                    break;
                }
            }
            e.setPaidBy(payer);
        }
        recurringExpenses.addExpense(e);
    }

    file.close();
}

void MainWindow::applyRecurringExpensesIfNeeded() {
    if (recurringExpenses.size() == 0) {
        return;
    }

    const QString month = QString("%1-%2")
        .arg(selectedFilterYear, 4, 10, QLatin1Char('0'))
        .arg(selectedFilterMonth, 2, 10, QLatin1Char('0'));

    // Remove stale recurring expenses for this month before re-adding
    for (int i = expenseList.size(); i >= 1; --i) {
        if (QString::fromStdString(expenseList.getExpense(i).getStatementMonth()) != month) {
            continue;
        }
        const std::string itemName = expenseList.getExpense(i).getItem();
        for (int j = 1; j <= recurringExpenses.size(); ++j) {
            if (recurringExpenses.getExpense(j).getItem() == itemName) {
                expenseList.deleteExpense(i);
                break;
            }
        }
    }

    const QString date = QDate(selectedFilterYear, selectedFilterMonth, 1).toString(Qt::ISODate);
    for (int i = 1; i <= recurringExpenses.size(); ++i) {
        Expense e = recurringExpenses.getExpense(i);
        e.setDate(date.toStdString());
        e.setStatementMonth(month.toStdString());
        expenseList.addExpense(e);
    }

    saveExpenseSettingsToDisk();
}

void MainWindow::openRecurringExpensesSettings() {
    QDialog dialog(this);
    dialog.setWindowTitle("Recurring expenses");
    dialog.resize(600, 400);

    QVBoxLayout* dialogLayout = new QVBoxLayout(&dialog);

    QTableWidget* table = new QTableWidget(&dialog);
    table->setColumnCount(5);
    table->setHorizontalHeaderLabels({"Item", "Amount", "Equal split", "Paid for", "Paid by"});
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->horizontalHeader()->setStretchLastSection(true);
    dialogLayout->addWidget(table);

    QFormLayout* formLayout = new QFormLayout();
    QLineEdit* itemEdit = new QLineEdit(&dialog);
    QDoubleSpinBox* amountEdit = new QDoubleSpinBox(&dialog);
    amountEdit->setRange(0.0, 10000000.0);
    amountEdit->setDecimals(2);
    QCheckBox* splitCheck = new QCheckBox("Equal split", &dialog);
    splitCheck->setChecked(true);
    QComboBox* paidForCombo = new QComboBox(&dialog);
    QComboBox* paidByCombo = new QComboBox(&dialog);
    paidForCombo->addItem("Both");
    for (int i = 1; i <= userList.size(); ++i) {
        const QString name = QString::fromStdString(userList.getUser(i).getName());
        paidForCombo->addItem(name);
        paidByCombo->addItem(name);
    }
    formLayout->addRow("Item", itemEdit);
    formLayout->addRow("Amount", amountEdit);
    formLayout->addRow("Paid for", paidForCombo);
    formLayout->addRow("Paid by", paidByCombo);
    formLayout->addRow("", splitCheck);
    dialogLayout->addLayout(formLayout);

    auto refreshTable = [&]() {
        table->setRowCount(recurringExpenses.size());
        for (int i = 1; i <= recurringExpenses.size(); ++i) {
            const Expense e = recurringExpenses.getExpense(i);
            table->setItem(i - 1, 0, new QTableWidgetItem(QString::fromStdString(e.getItem())));
            table->setItem(i - 1, 1, new QTableWidgetItem(QString::number(e.getAmount(), 'f', 2)));
            table->setItem(i - 1, 2, new QTableWidgetItem(e.isEqualSplit() ? "Yes" : "No"));
            table->setItem(i - 1, 3, new QTableWidgetItem(QString::fromStdString(e.getPaidFor().empty() ? "Both" : e.getPaidFor())));
            table->setItem(i - 1, 4, new QTableWidgetItem(QString::fromStdString(e.getPaidBy().getName().empty() ? "Unknown" : e.getPaidBy().getName())));
        }
    };

    QHBoxLayout* buttonsLayout = new QHBoxLayout();
    QPushButton* addButton = new QPushButton("Add", &dialog);
    QPushButton* removeButton = new QPushButton("Remove", &dialog);
    QPushButton* closeButton = new QPushButton("Close", &dialog);

    connect(addButton, &QPushButton::clicked, &dialog, [&]() {
        if (itemEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(&dialog, "Missing data", "Please enter an item name.");
            return;
        }
        Expense e;
        e.setItem(itemEdit->text().trimmed().toStdString());
        e.setAmount(amountEdit->value());
        e.setEqualSplit(splitCheck->isChecked());
        e.setPaidFor(paidForCombo->currentText().toStdString());
        const QString payerName = paidByCombo->currentText();
        for (int j = 1; j <= userList.size(); ++j) {
            const User candidate = userList.getUser(j);
            if (candidate.getName() == payerName.toStdString()) {
                e.setPaidBy(candidate);
                break;
            }
        }
        recurringExpenses.addExpense(e);
        saveRecurringExpensesToDisk();
        applyRecurringExpensesIfNeeded();
        saveExpenseSettingsToDisk();
        refreshExpenseList();
        refreshTable();
        itemEdit->clear();
        amountEdit->setValue(0.0);
    });

    connect(removeButton, &QPushButton::clicked, &dialog, [&]() {
        if (table->currentRow() < 0) {
            QMessageBox::warning(&dialog, "Selection needed", "Select a recurring expense first.");
            return;
        }
        // Purge all instances of this template from every month before removing the template
        const std::string deletedItem = recurringExpenses.getExpense(table->currentRow() + 1).getItem();
        for (int i = expenseList.size(); i >= 1; --i) {
            if (expenseList.getExpense(i).getItem() == deletedItem) {
                expenseList.deleteExpense(i);
            }
        }
        recurringExpenses.deleteExpense(table->currentRow() + 1);
        saveRecurringExpensesToDisk();
        applyRecurringExpensesIfNeeded();
        saveExpenseSettingsToDisk();
        refreshExpenseList();
        refreshTable();
    });

    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);

    buttonsLayout->addWidget(addButton);
    buttonsLayout->addWidget(removeButton);
    buttonsLayout->addWidget(closeButton);
    dialogLayout->addLayout(buttonsLayout);

    refreshTable();
    dialog.exec();
}
