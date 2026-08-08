#ifndef USERLIST_H
#define USERLIST_H

#include <string>
#include <map>
#include <iostream>
#include <vector>
#include "user.h"

class UserList {
public:
    void addUser(const User&);
    void displayUsers() const;
    void displayUsernames() const;
    void clearUsers();
    void deleteUser(int index);
    void modifyUser(int index, const std::string& newName, int newSalary, const std::string& newCardNumber);
    User getUser(int index) const;
    int size() const;
    void updateSalaryFactors();
    User getUserByCardIdentifier(const std::string& cardIdentifier) const;
private:
    std::vector<User> users;
};

#endif // USERLIST_H