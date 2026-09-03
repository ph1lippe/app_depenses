#include "settlement.h"
#include <QString>
#include <QDate>
#include <cmath>

static QDate parseDate(const QString& dateText) {
    if (dateText.isEmpty()) return QDate();
    QDate d = QDate::fromString(dateText, Qt::ISODate);
    if (d.isValid()) return d;
    d = QDate::fromString(dateText, "dd/MM/yyyy");
    if (d.isValid()) return d;
    d = QDate::fromString(dateText, "MM/dd/yyyy");
    if (d.isValid()) return d;
    d = QDate::fromString(dateText, "dd.MM.yyyy");
    if (d.isValid()) return d;
    d = QDate::fromString(dateText, "yyyy-MM");
    if (d.isValid()) return d;
    return QDate::fromString(dateText);
}

static bool expenseMatchesMonthYear(const Expense& expense, int selectedYear, int selectedMonth) {
    const QString statementMonth = QString::fromStdString(expense.getStatementMonth()).trimmed();
    if (!statementMonth.isEmpty()) {
        const QStringList parts = statementMonth.split('-');
        if (parts.size() >= 2) {
            bool yearOk = false;
            bool monthOk = false;
            const int statementYear = parts[0].toInt(&yearOk);
            const int statementMonthValue = parts[1].toInt(&monthOk);
            if (yearOk && monthOk) {
                return statementYear == selectedYear && statementMonthValue == selectedMonth;
            }
        }
    }

    const QString dateText = QString::fromStdString(expense.getDate()).trimmed();
    if (dateText.isEmpty()) {
        return true;
    }

    QDate parsedDate = parseDate(dateText);
    if (!parsedDate.isValid()) {
        return true;
    }

    return parsedDate.year() == selectedYear && parsedDate.month() == selectedMonth;
}

std::string computeSettlementResult(const UserList& userList, const ExpenseList& expenseList, int selectedYear, int selectedMonth) {
    if (userList.size() < 2) {
        return "Add at least two users to compute a split.";
    }
    // Ensure salary factors are up to date for weighted splits
    UserList ul = userList;
    ul.updateSalaryFactors();
    const User user1 = ul.getUser(1);
    const User user2 = ul.getUser(2);
    const QString user1Name = QString::fromStdString(user1.getName());
    const QString user2Name = QString::fromStdString(user2.getName());
    double balanceUser1 = 0.0;
    double balanceUser2 = 0.0;

    for (int i = 1; i <= expenseList.size(); ++i) {
        const Expense expense = expenseList.getExpense(i);
        if (!expenseMatchesMonthYear(expense, selectedYear, selectedMonth)) {
            continue;
        }

        const QString paidForText = QString::fromStdString(expense.getPaidFor().empty() ? "Both" : expense.getPaidFor());
        const QString payerName = QString::fromStdString(expense.getPaidBy().getName());
        const double amount = expense.getAmount();

        if (paidForText == "Both") {
            if (expense.isEqualSplit()) {
                if (payerName == user1Name) {
                    balanceUser1 += amount / 2.0;
                    balanceUser2 -= amount / 2.0;
                } else if (payerName == user2Name) {
                    balanceUser2 += amount / 2.0;
                    balanceUser1 -= amount / 2.0;
                }
            } else {
                // Weighted split by raw salaries: each participant pays proportional to their salary
                const double totalSalary = static_cast<double>(user1.getSalary()) + static_cast<double>(user2.getSalary());
                if (totalSalary <= 0.0) {
                    // Fallback to equal split
                    if (payerName == user1Name) {
                        balanceUser1 += amount / 2.0;
                        balanceUser2 -= amount / 2.0;
                    } else if (payerName == user2Name) {
                        balanceUser2 += amount / 2.0;
                        balanceUser1 -= amount / 2.0;
                    }
                } else {
                    if (payerName == user1Name) {
                        const double otherShare = amount * (static_cast<double>(user2.getSalary()) / totalSalary);
                        balanceUser1 += otherShare;
                        balanceUser2 -= otherShare;
                    } else if (payerName == user2Name) {
                        const double otherShare = amount * (static_cast<double>(user1.getSalary()) / totalSalary);
                        balanceUser2 += otherShare;
                        balanceUser1 -= otherShare;
                    }
                }
            }
        } else if (paidForText == user1Name) {
            if (payerName == user2Name) {
                balanceUser1 -= amount;
                balanceUser2 += amount;
            }
        } else if (paidForText == user2Name) {
            if (payerName == user1Name) {
                balanceUser1 += amount;
                balanceUser2 -= amount;
            }
        }
    }

    if (qAbs(balanceUser1) < 1e-9 && qAbs(balanceUser2) < 1e-9) {
        return "Everyone is settled.";
    }

    if (balanceUser1 > 1e-9) {
        return QString("%1 owes %2 to %3.")
            .arg(user2Name)
            .arg(QString::number(balanceUser1, 'f', 2))
            .arg(user1Name)
            .toStdString();
    }

    return QString("%1 owes %2 to %3.")
        .arg(user1Name)
        .arg(QString::number(-balanceUser1, 'f', 2))
        .arg(user2Name)
        .toStdString();
}
