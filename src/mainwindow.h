#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMenu>
#include <QAction>
#include <QDialog>
#include <QSlider>
#include <QVector>
#include <QSet>
#include <QEvent>

#include "userList.h"
#include "expenseList.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void addUser();
    void removeUser();
    void saveUser();
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
    void clearUserForm();
    void clearExpenseForm();
    void loadSelectedUser();
    void loadSelectedExpense();
    void clearExpenseFormForMultiSelection();
    void ensureComboPlaceholder(QComboBox* combo, const QString& placeholder);
    void removeComboPlaceholder(QComboBox* combo, const QString& placeholder);
    void saveToFile(const QString& filePath);
    void loadFromFile(const QString& filePath);
    bool eventFilter(QObject* watched, QEvent* event) override;
    QString getAppDataDirectoryPath() const;
    QString getUserSettingsFilePath() const;
    QString getExpenseSettingsFilePath() const;
    void saveUserSettingsToDisk();
    void loadUserSettingsFromDisk();
    void saveExpenseSettingsToDisk();
    void loadExpenseSettingsFromDisk();
    QString getRecurringExpensesFilePath() const;
    void saveRecurringExpensesToDisk();
    void loadRecurringExpensesFromDisk();
    void applyRecurringExpensesIfNeeded();
    bool expenseMatchesCurrentMonthYear(const Expense& expense) const;
    int getSelectedExpenseIndex() const;
    int getSelectedExpenseIndexForRow(int row) const;
    void saveExpense(ExpenseField field = ExpenseField::All);
    bool isIsoDateValid(const QString& dateText) const;
    QDate parseDate(const QString& dateText) const;

    UserList userList;
    ExpenseList expenseList;
    ExpenseList recurringExpenses;

    QListWidget* userListWidget;
    QTableWidget* expenseListWidget;
    QTextEdit* resultArea;

    QLineEdit* userNameEdit;
    QSpinBox* userSalaryEdit;
    QPushButton* addUserButton;
    QPushButton* removeUserButton;

    QLineEdit* expenseItemEdit;
    QDoubleSpinBox* expenseAmountEdit;
    QLineEdit* expenseDateEdit;
    QComboBox* expenseCardholderCombo;
    QComboBox* expensePayerCombo;
    QComboBox* expensePaidForCombo;
    QCheckBox* equalSplitCheck;
    QPushButton* addExpenseButton;
    QPushButton* removeExpenseButton;
    bool m_updatingExpenseForm{false};
    QSlider* monthSlider;
    QSpinBox* yearSpinBox;
    QLabel* monthFilterLabel;
    QVector<int> visibleExpenseIndices;
    int selectedFilterMonth;
    int selectedFilterYear;
    int lastSelectedExpenseIndex{ -1 };
};

#endif
