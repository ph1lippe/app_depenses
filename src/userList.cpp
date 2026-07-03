#include "userList.h"

void UserList::addUser(const User& user) {
    users.push_back(user);
}

void UserList::deleteUser(int index) {
    if (index < 1 || index > users.size()) {
        std::cout << "Invalid user index." << std::endl;
        return;
    }
    users.erase(users.begin() + index - 1);
}

void UserList::modifyUser(int index, const std::string& newName, int newSalary) {
    if (index < 1 || index > users.size()) {
        std::cout << "Invalid user index." << std::endl;
        return;
    }
    users[index - 1].setName(newName);
    users[index - 1].setSalary(newSalary);
}

void UserList::displayUsers() const {
    int count = 1;
    for (const auto& user : users) {
        std::cout << count << ". " << user.displayInfo();
        count++;
    }
}

void UserList::displayUsernames() const {
    int count = 1;
    for (const auto& user : users) {
        std::cout << count << ". " << user.getName() << std::endl;
        count++;
    }
}

User UserList::getUser(int index) const {
    if (index < 1 || index > users.size()) {
        std::cout << "Invalid user index." << std::endl;
        return User("Unknown", 0);
    }
    return users[index - 1];
}

void UserList::updateSalaryFactors() {
    int meanSalary = 0;
    for (const auto& user : users) {
        meanSalary += user.getSalary();
    }
    meanSalary /= static_cast<int>(users.size());
    for (auto& user : users) {
        if (meanSalary > 0) {
            user.setSalaryFactor(static_cast<double>(user.getSalary()) / meanSalary);
        } else {
            user.setSalaryFactor(0.0);
        }
    }
}