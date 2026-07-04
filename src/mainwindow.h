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
    void saveExpense();
    void computeSplit();
    void saveStateToFile();
    void loadStateFromFile();
    void openUserSettings();
    void importExpensesFromFile();
    void updateExpenseViewFilter();

private:
    void refreshUserList();
    void refreshExpenseList();
    void clearUserForm();
    void clearExpenseForm();
    void loadSelectedUser();
    void loadSelectedExpense();
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
    bool expenseMatchesCurrentMonthYear(const Expense& expense) const;
    int getSelectedExpenseIndex() const;
    int getSelectedExpenseIndexForRow(int row) const;

    UserList userList;
    ExpenseList expenseList;

    QListWidget* userListWidget;
    QTableWidget* expenseListWidget;
    QTextEdit* resultArea;

    QLineEdit* userNameEdit;
    QSpinBox* userSalaryEdit;
    QPushButton* addUserButton;
    QPushButton* removeUserButton;

    QLineEdit* expenseItemEdit;
    QDoubleSpinBox* expenseAmountEdit;
    QComboBox* expensePayerCombo;
    QComboBox* expensePaidForCombo;
    QCheckBox* equalSplitCheck;
    QPushButton* addExpenseButton;
    QPushButton* removeExpenseButton;
    QSlider* monthSlider;
    QSpinBox* yearSpinBox;
    QLabel* monthFilterLabel;
    QVector<int> visibleExpenseIndices;
    int selectedFilterMonth;
    int selectedFilterYear;
};

#endif
