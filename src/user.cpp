#include "user.h"
User::User() : name(""), salary(0) {}
User::User(const std::string& name) : name(name), salary(0) {}
User::User(const std::string& name, int salary) : name(name), salary(salary) {}

std::string User::getName() const {return name;}
int User::getSalary() const {return salary;}
void User::setName(const std::string& name) {this->name = name;}
void User::setSalary(int salary) {this->salary = salary;}

std::string User::displayInfo() const {
    std::string info = "Name: " + name + ", Salary: " + std::to_string(salary);
    return info;
}

void User::inputUserData() {
    std::string name;
    int salary;

    std::cout << "Enter name: ";
    std::cin >> name;
    setName(name);

    std::cout << "Enter salary: ";
    std::cin >> salary;
    setSalary(salary);
}

void User::setSalaryFactor(double factor) {salaryFactor = factor; }
double User::getSalaryFactor() const { return salaryFactor; }