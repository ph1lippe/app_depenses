#ifndef EXPENSE_MANAGER_H
#define EXPENSE_MANAGER_H

#include "userList.h"
#include "expenseList.h"

class ExpenseManager {
public:
    void mainMenu();
    UserList& getUserList();
private:
    UserList userList;
    ExpenseList expenseList;

    int displayMain();
    void userMenu();
    int displayUserMenu();
    void manageExpenses();
    int displayManageExpenseMenu();
    void computeSplit();
};

#endif // EXPENSE_MANAGER_H