#ifndef USER_H
#define USER_H

#include <string>
#include <iostream>

class User {
public:
    User();
    User(const std::string& name);
    User(const std::string& name, int salary);
    std::string displayInfo() const;
    std::string getName() const;
    int getSalary() const;
    void inputUserData();
    void setName(const std::string&);
    void setSalary(int);
    void setSalaryFactor(double factor);
    double getSalaryFactor() const;
private:
    std::string name;
    int salary;
    double salaryFactor;
};

#endif // USER_H
