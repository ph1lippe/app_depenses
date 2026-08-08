#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QEvent>

#include "userList.h"
#include "expenseList.h"
#include "appdata_persistence.h"

class QCheckBox;
class QDate;

class ExpenseEditorWidget;
class ExpenseTableWidget;
class MonthFilterWidget;
class SettlementResultWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void addExpense();
    void removeExpense();
    void computeSplit();
    void saveStateToFile();
    void loadStateFromFile();
    void openUserSettings();
    void openRecurringExpensesSettings();
    void importExpensesFromFile();
    void updateExpenseViewFilter();

private:
    enum class ExpenseField {
        Item,
        Amount,
        Date,
        Cardholder,
        PaidBy,
        PaidFor,
        EqualSplit,
        All
    };
    void refreshUserList();
    void refreshExpenseList();
    void clearExpenseForm();
    void loadSelectedExpense();
    void clearExpenseFormForMultiSelection();
    void saveToFile(const QString& filePath);
    void loadFromFile(const QString& filePath);
    bool eventFilter(QObject* watched, QEvent* event) override;
    void saveUserSettingsToDisk();
    void loadUserSettingsFromDisk();
    void saveExpenseSettingsToDisk();
    void loadExpenseSettingsFromDisk();
    void saveRecurringExpensesToDisk();
    void loadRecurringExpensesFromDisk();
    void applyRecurringExpensesIfNeeded();
    bool expenseMatchesCurrentMonthYear(const Expense& expense) const;
    int getSelectedExpenseIndex() const;
    int getSelectedExpenseIndexForRow(int row) const;
    int findVisibleRowForExpenseIndex(int expenseIndex) const;
    void saveExpense(ExpenseField field = ExpenseField::All);
    bool isIsoDateValid(const QString& dateText) const;
    QDate parseDate(const QString& dateText) const;

    UserList userList;
    ExpenseList expenseList;
    ExpenseList recurringExpenses;

    ExpenseTableWidget* expenseListWidget;
    ExpenseEditorWidget* expenseEditorWidget;
    MonthFilterWidget* monthFilterWidget;
    SettlementResultWidget* settlementResultWidget;

    bool m_updatingExpenseForm{false};
    QVector<int> visibleExpenseIndices;
    int selectedFilterMonth;
    int selectedFilterYear;
    int lastSelectedExpenseIndex{ -1 };
};

#endif
