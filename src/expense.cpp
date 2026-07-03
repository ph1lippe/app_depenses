#include "expense.h"
#include <iostream>
#include <string>

Expense::Expense() : item("Unknown"), amount(0.0), user("Unknown") {}
Expense::Expense(const std::string& item) : item(item), amount(0.0), user("Unknown") {}
Expense::Expense(const std::string& item, double amount) : item(item), amount(amount), user("Unknown") {}
Expense::Expense(const std::string& item, double amount, const User& user)
    : item(item), amount(amount), user(user) {}

void Expense::display() const {
    std::cout << "Expense Item: " << item << ", Amount: " << amount << ", User: " << user.getName() << std::endl;
}

void Expense::inputExpenseData(UserList& userList) {
    std::string item;
    std::cout << "Enter expense item: ";
    std::cin >> item;
    this->item = item;

    double amount;
    std::cout << "Enter expense amount: ";
    std::cin >> amount;
    this->amount = amount;

    userList.displayUsernames();
    int index;
    std::cout << "Select user: " << std::endl;
    std::cin >> index;
    user = userList.getUser(index);
    this->user = user;

    bool equalSplit;
    std::string response;

    bool validInput = false;
    while (!validInput){
        std::cout << "Is this expense split equally? (y/n): ";
        
        std::cin >> response;
        if (response != "y" && response != "n") {
            std::cout << "Invalid input, defaulting to equal split." << std::endl;
        } else { validInput = true;}
        equalSplit = (response == "y");
        this->equalSplit = equalSplit;
    }
}

User Expense::getPaidBy() const {return user; }
double Expense::getAmount() const { return amount; }
std::string Expense::getItem() const { return item; }
bool Expense::isEqualSplit() const { return equalSplit; }
void Expense::setItem(const std::string& value) { item = value; }
void Expense::setAmount(double value) { amount = value; }
void Expense::setPaidBy(const User& value) { user = value; }
void Expense::setEqualSplit(bool value) { equalSplit = value; }