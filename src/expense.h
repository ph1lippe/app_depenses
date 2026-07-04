#ifndef EXPENSE_H
#define EXPENSE_H

#include <string>
#include <iostream>
#include "user.h"
#include "userList.h"

class Expense {
public:
    Expense();
    Expense(const std::string&);
    Expense(const std::string&, double);
    Expense(const std::string&, double, const User&);
    void display() const;
    void inputExpenseData(UserList& userList);
    User getPaidBy() const;
    double getAmount() const;
    std::string getItem() const;
    std::string getCardIdentifier() const;
    std::string getDate() const;
    std::string getStatementMonth() const;
    std::string getPaidFor() const;
    bool isEqualSplit() const;
    void setItem(const std::string&);
    void setAmount(double);
    void setPaidBy(const User&);
    void setCardIdentifier(const std::string&);
    void setDate(const std::string&);
    void setStatementMonth(const std::string&);
    void setPaidFor(const std::string&);
    void setEqualSplit(bool value);

private:
    std::string item;
    double amount;
    User user;
    std::string cardIdentifier;
    std::string date;
    std::string statementMonth;
    std::string paidFor;
    bool equalSplit;
};

#endif // EXPENSE_H