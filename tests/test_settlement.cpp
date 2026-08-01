#include <cassert>
#include <iostream>
#include "userList.h"
#include "expenseList.h"
#include "settlement.h"

int main() {
    UserList userList;
    userList.addUser(User("Alice", 1000));
    userList.addUser(User("Bob", 1000));

    Expense expense;
    expense.setItem("Dinner");
    expense.setAmount(10.0);
    expense.setPaidBy(userList.getUser(1));
    expense.setPaidFor("Bob");
    expense.setEqualSplit(true);
    expense.setDate("2026-07-31");
    expense.setStatementMonth("2026-07");
    expense.setCardholder("Alice");

    ExpenseList expenseList;
    expenseList.addExpense(expense);

    const std::string result = computeSettlementResult(userList, expenseList, 2026, 7);
    const std::string expected = "Bob owes 5.00 to Alice.";

    if (result != expected) {
        std::cerr << "Expected: " << expected << "\n";
        std::cerr << "Actual:   " << result << "\n";
        return 1;
    }

    std::cout << "Test passed: " << result << "\n";
    return 0;
}
