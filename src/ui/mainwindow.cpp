#include "mainwindow.h"
#include "MonthFilterWidget.h"
#include "ExpenseEditorWidget.h"
#include "ExpenseTableWidget.h"
#include "SettlementResultWidget.h"
#include "UserSettingsDialog.h"
#include "RecurringExpensesDialog.h"
#include "settlement.h"
#include <QMessageBox>
#include <QStringList>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include <QMenuBar>
#include <QFileInfo>
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
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
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
      expenseListWidget(nullptr),
      expenseEditorWidget(nullptr),
      monthFilterWidget(nullptr),
      settlementResultWidget(nullptr),
      selectedFilterMonth(1),
      selectedFilterYear(QDate::currentDate().year()) {
    QWidget* central = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(central);

    QGroupBox* monthFilterGroup = new QGroupBox("Expense month filter", this);
    QHBoxLayout* monthFilterLayout = new QHBoxLayout(monthFilterGroup);
    monthFilterWidget = new MonthFilterWidget(this);
    monthFilterWidget->setMonth(selectedFilterMonth);
    monthFilterWidget->setYear(selectedFilterYear);
    monthFilterLayout->addWidget(monthFilterWidget);
    monthFilterLayout->addStretch();
    connect(monthFilterWidget, &MonthFilterWidget::filterChanged, this, [this](int month, int year) {
        selectedFilterMonth = month;
        selectedFilterYear = year;
        updateExpenseViewFilter();
    });

    QGroupBox* expensesGroup = new QGroupBox("Expense editor", this);
    QVBoxLayout* expensesLayout = new QVBoxLayout(expensesGroup);

    // Box 2: expense list
    expenseListWidget = new ExpenseTableWidget(this);
    connect(expenseListWidget, &QTableWidget::cellClicked, this, [this](int row, int column) {
        Q_UNUSED(column);
        if (row >= 0) {
            if (!(QApplication::keyboardModifiers() & Qt::ControlModifier) && !(QApplication::keyboardModifiers() & Qt::ShiftModifier)) {
                expenseListWidget->selectRow(row);
            }
            loadSelectedExpense();
        }
    });
    connect(expenseListWidget, &ExpenseTableWidget::rowSelectionChanged, this, [this](int) {
        lastSelectedExpenseIndex = getSelectedExpenseIndex();
    });
    expenseListWidget->installEventFilter(this);
    expensesLayout->addWidget(expenseListWidget);

    expenseEditorWidget = new ExpenseEditorWidget(this);
    connect(expenseEditorWidget, &ExpenseEditorWidget::itemEdited, this, [this]() { if (!m_updatingExpenseForm) saveExpense(ExpenseField::Item); });
    connect(expenseEditorWidget, &ExpenseEditorWidget::amountEdited, this, [this]() { if (!m_updatingExpenseForm) saveExpense(ExpenseField::Amount); });
    connect(expenseEditorWidget, &ExpenseEditorWidget::dateEdited, this, [this]() { if (!m_updatingExpenseForm) saveExpense(ExpenseField::Date); });
    connect(expenseEditorWidget, &ExpenseEditorWidget::cardholderChanged, this, [this](int) { if (!m_updatingExpenseForm) saveExpense(ExpenseField::Cardholder); });
    connect(expenseEditorWidget, &ExpenseEditorWidget::payerChanged, this, [this](int) { if (!m_updatingExpenseForm) saveExpense(ExpenseField::PaidBy); });
    connect(expenseEditorWidget, &ExpenseEditorWidget::paidForChanged, this, [this](int) { if (!m_updatingExpenseForm) saveExpense(ExpenseField::PaidFor); });
    connect(expenseEditorWidget, &ExpenseEditorWidget::equalSplitToggled, this, [this](bool) { if (!m_updatingExpenseForm) saveExpense(ExpenseField::EqualSplit); });
    connect(expenseEditorWidget, &ExpenseEditorWidget::addExpenseRequested, this, &MainWindow::addExpense);
    connect(expenseEditorWidget, &ExpenseEditorWidget::removeExpenseRequested, this, &MainWindow::removeExpense);
    expensesLayout->addWidget(expenseEditorWidget);

    // Box 3: Settlement result
    QGroupBox* splitGroup = new QGroupBox("Settlement result", this);
    QVBoxLayout* splitLayout = new QVBoxLayout(splitGroup);
    settlementResultWidget = new SettlementResultWidget(this);
    splitLayout->addWidget(settlementResultWidget);

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
void MainWindow::saveUserSettingsToDisk() {
    app_persistence::saveUsers(userList);
}

void MainWindow::loadUserSettingsFromDisk() {
    app_persistence::loadUsers(userList);
}

void MainWindow::saveExpenseSettingsToDisk() {
    app_persistence::saveExpenses(expenseList);
}

void MainWindow::loadExpenseSettingsFromDisk() {
    app_persistence::loadExpenses(expenseList, userList, QString(), selectedFilterYear, selectedFilterMonth, [this](const QString& text) {
        return parseDate(text);
    });

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
                    if (monthFilterWidget) {
                        monthFilterWidget->setYear(selectedFilterYear);
                        monthFilterWidget->setMonth(selectedFilterMonth);
                    }
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

    if (monthFilterWidget) {
        selectedFilterYear = monthFilterWidget->year();
        selectedFilterMonth = monthFilterWidget->month();
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
    QStringList userNames;
    for (int i = 1; i <= userList.size(); ++i) {
        const User user = userList.getUser(i);
        userNames.append(QString::fromStdString(user.getName()));
    }

    if (expenseEditorWidget) {
        expenseEditorWidget->setUserOptions(userNames);
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

    const bool wasSorting = expenseListWidget->isSortingEnabled();
    int sortColumn = -1;
    Qt::SortOrder sortOrder = Qt::AscendingOrder;
    if (wasSorting && expenseListWidget->horizontalHeader()) {
        sortColumn = expenseListWidget->horizontalHeader()->sortIndicatorSection();
        sortOrder = expenseListWidget->horizontalHeader()->sortIndicatorOrder();
        expenseListWidget->setSortingEnabled(false);
    }

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

    if (wasSorting) {
        expenseListWidget->setSortingEnabled(true);
        if (expenseListWidget->horizontalHeader()) {
            expenseListWidget->horizontalHeader()->setSortIndicator(sortColumn, sortOrder);
        }
    }

    bool selectionRestored = false;
    if (previouslySelectedExpenseIndex > 0) {
        const int rowToSelect = findVisibleRowForExpenseIndex(previouslySelectedExpenseIndex);
        if (rowToSelect >= 0) {
            expenseListWidget->selectionModel()->select(
                expenseListWidget->model()->index(rowToSelect, 0),
                QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            expenseListWidget->setCurrentCell(rowToSelect, 0);
            lastSelectedExpenseIndex = previouslySelectedExpenseIndex;
            selectionRestored = true;
        }
    }

    const QModelIndexList selRows = expenseListWidget->selectionModel()->selectedRows(0);
    if (selRows.isEmpty() && !visibleExpenseIndices.isEmpty()) {
        expenseListWidget->selectionModel()->select(
            expenseListWidget->model()->index(0, 0),
            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        expenseListWidget->setCurrentCell(0, 0);
        lastSelectedExpenseIndex = visibleExpenseIndices.value(0);
    }

    const QModelIndexList selectedRows = expenseListWidget->selectionModel()->selectedRows(0);
    if (selectedRows.size() == 1) {
        loadSelectedExpense();
    } else if (selectedRows.size() > 1) {
        clearExpenseFormForMultiSelection();
    } else {
        clearExpenseForm();
    }

    if (settlementResultWidget) {
        computeSplit();
    }
}

void MainWindow::clearExpenseForm() {
    if (expenseEditorWidget) {
        expenseEditorWidget->clearForm();
    }
}

void MainWindow::clearExpenseFormForMultiSelection() {
    if (expenseEditorWidget) {
        expenseEditorWidget->clearForMultiSelection();
    }
}


// Expense methods:
void MainWindow::addExpense() {
    if (userList.size() <= 0) {
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
    if (expenseListWidget && expenseListWidget->selectionModel()->selectedRows(0).isEmpty()) {
        clearExpenseForm();
    }
}

void MainWindow::saveExpense(ExpenseField field) {
    const QModelIndexList selectedRows = expenseListWidget->selectionModel()->selectedRows(0);
    if (selectedRows.isEmpty() || !expenseEditorWidget) {
        return;
    }

    User payer("Unknown", 0);
    const QString payerText = expenseEditorWidget->payerText();
    if (!payerText.isEmpty()) {
        for (int i = 1; i <= userList.size(); ++i) {
            const User user = userList.getUser(i);
            if (QString::fromStdString(user.getName()) == payerText) {
                payer = user;
                break;
            }
        }
    }

    const QString amountText = QString::number(expenseEditorWidget->amount(), 'f', 2);
    const QString dateText = expenseEditorWidget->dateText();
    QString cardholderText = expenseEditorWidget->cardholderText();
    if (cardholderText == "(multiple)") {
        cardholderText.clear();
    }
    const QString paidForText = expenseEditorWidget->paidForText();
    const QString itemText = expenseEditorWidget->itemText();
    const QString equalSplitText = expenseEditorWidget->equalSplit() ? "Yes" : "No";

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
                expense.setAmount(expenseEditorWidget->amount());
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
                if (!payerText.isEmpty()) {
                    expense.setPaidBy(payer);
                    expenseListWidget->item(row, 4)->setText(payerText);
                }
                break;
            case ExpenseField::PaidFor:
                expense.setPaidFor(paidForText.toStdString());
                expenseListWidget->item(row, 5)->setText(paidForText);
                break;
            case ExpenseField::EqualSplit:
                expense.setEqualSplit(expenseEditorWidget->equalSplit());
                expenseListWidget->item(row, 1)->setText(equalSplitText);
                break;
            case ExpenseField::All:
                expense.setItem(itemText.toStdString());
                expense.setAmount(expenseEditorWidget->amount());
                expense.setDate(dateText.toStdString());
                expense.setCardholder(cardholderText == "(multiple)" ? std::string() : cardholderText.toStdString());
                expense.setEqualSplit(expenseEditorWidget->equalSplit());
                expense.setPaidFor(paidForText.toStdString());
                if (!payerText.isEmpty()) {
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

    if (settlementResultWidget) {
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

void MainWindow::loadSelectedExpense() {
    const QModelIndexList selectedRows = expenseListWidget->selectionModel()->selectedRows(0);
    if (selectedRows.isEmpty() || !expenseEditorWidget) {
        return;
    }
    if (selectedRows.size() > 1) {
        clearExpenseFormForMultiSelection();
        return;
    }
    const int actualIndex = getSelectedExpenseIndex();
    if (actualIndex <= 0) {
        return;
    }
    // remember last selected expense index so selection can be restored across refreshes/filters
    lastSelectedExpenseIndex = actualIndex;
    const Expense expense = expenseList.getExpense(actualIndex);
    expenseEditorWidget->setItemText(QString::fromStdString(expense.getItem()));
    expenseEditorWidget->setDateText(QString::fromStdString(expense.getDate()));
    expenseEditorWidget->setAmount(expense.getAmount());
    expenseEditorWidget->setEqualSplit(expense.isEqualSplit());
    expenseEditorWidget->setCardholder(QString::fromStdString(expense.getCardholder()));
    expenseEditorWidget->setPayer(QString::fromStdString(expense.getPaidBy().getName()));
    expenseEditorWidget->setPaidFor(QString::fromStdString(expense.getPaidFor().empty() ? "Both" : expense.getPaidFor()));
}


void MainWindow::openUserSettings() {
    UserSettingsDialog dialog(userList, this);
    connect(&dialog, &UserSettingsDialog::usersChanged, this, [&](){
        saveUserSettingsToDisk();
        refreshUserList();
        computeSplit();
    });
    dialog.exec();
}

void MainWindow::saveStateToFile() {
    const QString filePath = app_persistence::getExpenseSettingsFilePath();
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
    clearExpenseForm();
    if (settlementResultWidget) {
        settlementResultWidget->setResultText("");
    }
    QMessageBox::information(this, "Loaded", "The saved users and expenses were loaded successfully.");
}

void MainWindow::updateExpenseViewFilter() {
    if (monthFilterWidget) {
        selectedFilterYear = monthFilterWidget->year();
        selectedFilterMonth = monthFilterWidget->month();
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

int MainWindow::findVisibleRowForExpenseIndex(int expenseIndex) const {
    if (!expenseListWidget || expenseIndex <= 0) {
        return -1;
    }
    for (int row = 0; row < expenseListWidget->rowCount(); ++row) {
        const QTableWidgetItem* item = expenseListWidget->item(row, 0);
        if (item && item->data(Qt::UserRole).toInt() == expenseIndex) {
            return row;
        }
    }
    return -1;
}

void MainWindow::computeSplit() {
    const std::string result = computeSettlementResult(userList, expenseList, selectedFilterYear, selectedFilterMonth);
    if (settlementResultWidget) {
        settlementResultWidget->setResultText(QString::fromStdString(result));
    }
}

// Recurring expenses:
void MainWindow::saveRecurringExpensesToDisk() {
    app_persistence::saveRecurringExpenses(recurringExpenses);
}

void MainWindow::loadRecurringExpensesFromDisk() {
    app_persistence::loadRecurringExpenses(recurringExpenses);
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
    RecurringExpensesDialog dialog(userList, recurringExpenses, this);
    connect(&dialog, &RecurringExpensesDialog::recurringExpensesChanged, this, [&]() {
        saveRecurringExpensesToDisk();
        applyRecurringExpensesIfNeeded();
        saveExpenseSettingsToDisk();
        refreshExpenseList();
    });
    dialog.exec();
}
