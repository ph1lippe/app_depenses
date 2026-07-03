#include "expenseList.h"

void ExpenseList::addExpense(const Expense& expense) {
    expenses.push_back(expense);
}

void ExpenseList::displayExpenses() const {
    int count = 1;
    for (const auto& expense : expenses) {
        std::cout << count++ << ". ";
        expense.display();
    }
}

void ExpenseList::clearExpenses() {
    expenses.clear();
}

void ExpenseList::deleteExpense(int index) {
    index--;
    if (index >= 0 && index < expenses.size()) {
        expenses.erase(expenses.begin() + index);
    }
    else {
        std::cout << "Invalid choice." << std::endl;
    }
}

void ExpenseList::modifyExpense(int index, UserList& userList) {
    index--;
    if (index >= 0 && index < expenses.size()) {
        Expense& expense = expenses[index];
        expense.inputExpenseData(userList);
    }
    else{
        std::cout << "Invalid choice." << std::endl;
    }
}

std::vector<Expense> ExpenseList::getExpenses() const {
    return expenses;
}

void ExpenseList::setAllEqualSplit(bool value) {
    for (auto& expense : expenses) {
        expense.setEqualSplit(value);
    }
}
