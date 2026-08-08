#ifndef RECURRINGEXPENSESDIALOG_H
#define RECURRINGEXPENSESDIALOG_H

#include <QDialog>

class UserList;
class ExpenseList;
class QTableWidget;
class QLineEdit;
class QDoubleSpinBox;
class QCheckBox;
class QComboBox;
class QPushButton;

class RecurringExpensesDialog : public QDialog {
    Q_OBJECT

public:
    explicit RecurringExpensesDialog(UserList& userList,
                                     ExpenseList& recurringExpenses,
                                     QWidget* parent = nullptr);

signals:
    void recurringExpensesChanged();

private slots:
    void onUserRowSelected(int row, int column);
    void addRecurringExpense();
    void removeRecurringExpense();

private:
    void refreshTable();
    void clearForm();
    void populateUserCombos();

    UserList& m_userList;
    ExpenseList& m_recurringExpenses;

    QTableWidget* table;
    QLineEdit* itemEdit;
    QDoubleSpinBox* amountEdit;
    QCheckBox* splitCheck;
    QComboBox* paidForCombo;
    QComboBox* paidByCombo;
    QPushButton* addButton;
    QPushButton* removeButton;
    QPushButton* closeButton;
};

#endif // RECURRINGEXPENSESDIALOG_H
