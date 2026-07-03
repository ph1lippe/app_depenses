#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
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

private:
    void refreshUserList();
    void refreshExpenseList();
    void clearUserForm();
    void clearExpenseForm();
    void loadSelectedUser();
    void loadSelectedExpense();

    UserList userList;
    ExpenseList expenseList;

    QListWidget* userListWidget;
    QListWidget* expenseListWidget;
    QTextEdit* resultArea;

    QLineEdit* userNameEdit;
    QSpinBox* userSalaryEdit;
    QPushButton* addUserButton;
    QPushButton* removeUserButton;
    QPushButton* saveUserButton;

    QLineEdit* expenseItemEdit;
    QDoubleSpinBox* expenseAmountEdit;
    QComboBox* expensePayerCombo;
    QCheckBox* equalSplitCheck;
    QPushButton* addExpenseButton;
    QPushButton* removeExpenseButton;
    QPushButton* saveExpenseButton;
    QPushButton* computeButton;
};

#endif
