#include <cassert>
#include <iostream>
#include "userList.h"
#include "expenseList.h"
#include "settlement.h"

int main() {
    UserList userList;
    userList.addUser(User("alice", 10));
    userList.addUser(User("bob", 20));

    Expense expense;
    expense.setItem("apple");
    expense.setAmount(9.0);
    expense.setPaidBy(userList.getUser(1));
    expense.setPaidFor("Both");
    expense.setEqualSplit(false);
    expense.setDate("2025-01-03");
    expense.setStatementMonth("2025-01");
    expense.setCardholder("alice");

    ExpenseList expenseList;
    expenseList.addExpense(expense);

    const std::string result = computeSettlementResult(userList, expenseList, 2025, 1);
    const std::string expected = "bob owes 6.00 to alice.";

    if (result != expected) {
        std::cerr << "Expected: " << expected << "\n";
        std::cerr << "Actual:   " << result << "\n";
        return 1;
    }

    std::cout << "Test passed: " << result << "\n";
    return 0;
}
