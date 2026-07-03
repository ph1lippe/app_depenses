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
    void deleteUser(int index);
    void modifyUser(int index, const std::string& newName, int newSalary);
    User getUser(int index) const;
    void updateSalaryFactors();
private:
    std::vector<User> users;
};

#endif // USERLIST_H