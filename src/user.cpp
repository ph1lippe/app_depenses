#include "user.h"
#include <cctype>

User::User() : name(""), salary(0), cardNumber("") {}
User::User(const std::string& name) : name(name), salary(0), cardNumber("") {}
User::User(const std::string& name, int salary) : name(name), salary(salary), cardNumber("") {}

std::string User::getName() const {return name;}
int User::getSalary() const {return salary;}
std::string User::getCardNumber() const { return cardNumber; }
std::string User::getCardIdentifier() const { return cardNumber; }
void User::setName(const std::string& name) {this->name = name;}
void User::setSalary(int salary) {this->salary = salary;}
void User::setCardNumber(const std::string& number) {
    cardNumber.clear();
    for (char ch : number) {
        if (std::isdigit(static_cast<unsigned char>(ch))) {
            cardNumber.push_back(ch);
        }
    }
    if (cardNumber.size() > 4) {
        cardNumber = cardNumber.substr(0, 4);
    }
}
void User::setCardIdentifier(const std::string& identifier) { setCardNumber(identifier); }

std::string User::displayInfo() const {
    std::string info = "Name: " + name + ", Salary: " + std::to_string(salary) + ", Card: " + cardNumber;
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