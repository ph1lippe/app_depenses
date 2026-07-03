#ifndef EXPENSE_LIST_H
#define EXPENSE_LIST_H

#include "expense.h"
#include <vector>

class ExpenseList {
public:
    void addExpense(const Expense& expense);
    void displayExpenses() const;
    void clearExpenses();
    void deleteExpense(int index);
    void modifyExpense(int index, UserList& userList);
    std::vector<Expense> getExpenses() const;
    int size() const;
    Expense getExpense(int index) const;
    void updateExpense(int index, const Expense& expense);
    void setAllEqualSplit(bool value);
private:
    std::vector<Expense> expenses;
};
#endif // EXPENSE_LIST_H