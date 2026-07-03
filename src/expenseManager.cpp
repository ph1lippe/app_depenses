#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "expenseManager.h"
#include "user.h"
#include "expense.h"

UserList& ExpenseManager::getUserList() {
    return userList;
}

// TODO: try making a template class for menu options/actions
void ExpenseManager::mainMenu() {
    int selection;
    bool exitFlag = false;
    while (!exitFlag) {
        selection = ExpenseManager::displayMain();
        if (selection == 1) {
            ExpenseManager::userMenu();
        } else if (selection == 2) {
            ExpenseManager::manageExpenses();
            //expenseManager.run();
        } else if (selection == 3) {
            ExpenseManager::computeSplit();
        } else if (selection == 4) {
            exitFlag = true;
        }
    }
    return;
}


int ExpenseManager::displayMain() {
    int selection;
    std::cout << "Select an option: " << std::endl;
    std::cout << "1. User settings." << std::endl;
    std::cout << "2. Manage expenses." << std::endl;
    std::cout << "3. Compute split." << std::endl;
    std::cout << "4. Exit." << std::endl;
    std::cin >> selection;
    // TODO: add input validation
    return selection;
}

void ExpenseManager::userMenu() {
    int selection;
    bool exitFlag = false;
    while (!exitFlag) {
        selection = ExpenseManager::displayUserMenu();
        if (selection == 1) {
            User user;
            user.inputUserData();
            userList.addUser(user);
        } else if (selection == 2) {
            userList.displayUsers();
            int selection;
            std::cout << "Enter the number of the user to delete: ";
            std::cin >> selection;
            userList.deleteUser(selection);
        } else if (selection == 3) {
            userList.displayUsers();
            int selection;
            std::cout << "Enter the number of the user to modify: ";
            std::cin >> selection;
            std::string newName;
            int newSalary;
            std::cout << "Enter new name: ";
            std::cin >> newName;
            std::cout << "Enter new salary: ";
            std::cin >> newSalary;
            userList.modifyUser(selection, newName, newSalary);
        } else if (selection == 4) {
            userList.displayUsers();
            std::cout << std::endl << "Press enter to continue...";
            std::cin.ignore();
            std::cin.get();
        } else if (selection == 5) {
            exitFlag = true;
        }
    }
    return;
}

int ExpenseManager::displayUserMenu() {
    int selection;
    std::cout << "Select an option: " << std::endl;
    std::cout << "1. Add user." << std::endl;
    std::cout << "2. Delete user." << std::endl;
    std::cout << "3. Modify user." << std::endl;
    std::cout << "4. Display users." << std::endl;
    std::cout << "5. Return to main menu." << std::endl;
    std::cin >> selection;
    // TODO: add input validation
    return selection;
}

int ExpenseManager::displayManageExpenseMenu() {
    int selection;
    std::cout << "Select an option: " << std::endl;
    std::cout << "1. Add expense." << std::endl;
    std::cout << "2. Delete expense." << std::endl;
    std::cout << "3. Modify expense." << std::endl;
    std::cout << "4. Display expenses." << std::endl;
    std::cout << "5. Return to main menu." << std::endl;
    std::cin >> selection;
    // TODO: add input validation
    return selection;
}

void ExpenseManager::manageExpenses() {
    int selection;
    bool exitFlag = false;
    while (!exitFlag) {
        selection = ExpenseManager::displayManageExpenseMenu();
        if (selection == 1) {
            Expense expense;
            UserList& userList = ExpenseManager::getUserList();
            expense.inputExpenseData(userList);
            expenseList.addExpense(expense);
        } else if (selection == 2) {
            expenseList.displayExpenses();
            int index;
            std::cout << "Enter expense index to delete: ";
            std::cin >> index;
            expenseList.deleteExpense(index);
        } else if (selection == 3) {
            expenseList.displayExpenses();
            int index;
            std::cout << "Enter expense index to modify: ";
            std::cin >> index;
            UserList& userList = ExpenseManager::getUserList();
            expenseList.modifyExpense(index, userList);
        } else if (selection == 4) {
            expenseList.displayExpenses();
            std::cout << std::endl << "Press enter to continue...";
            std::cin.ignore();
            std::cin.get();
        } else if (selection == 5) {
            exitFlag = true;
        }
    }
    return;
}

void ExpenseManager::computeSplit() {
    double paidByUser1 = 0.0;
    double paidByUser2 = 0.0;

    UserList& userList = ExpenseManager::getUserList();
    userList.updateSalaryFactors();

    for (const auto& expense : expenseList.getExpenses()) {
        if (expense.getPaidBy().getName() == userList.getUser(1).getName()) {
            if (expense.isEqualSplit()) {
                paidByUser1 += expense.getAmount();
            } else {
                paidByUser1 += expense.getAmount() * userList.getUser(2).getSalaryFactor();
            }
        } else if (expense.getPaidBy().getName() == userList.getUser(2).getName()) {
            if (expense.isEqualSplit()) {
                paidByUser2 += expense.getAmount();
            } else {
                paidByUser2 += expense.getAmount() * userList.getUser(1).getSalaryFactor();
            }
        }
    }

    double totalPaid = paidByUser1 + paidByUser2;
    if (totalPaid == 0) {
        std::cout << "No expenses to split." << std::endl;
        return;
    }

    double splitAmount = totalPaid / 2;

    if (paidByUser1 > splitAmount) {
        std::cout << userList.getUser(1).getName() << " is owed: " << (paidByUser1 - splitAmount) << std::endl;
    } else if (paidByUser2 > splitAmount) {
        std::cout << userList.getUser(2).getName() << " is owed: " << (paidByUser2 - splitAmount) << std::endl;
    } else {
        std::cout << "Both users are settled." << std::endl;
    }
}
