#include "appdata_persistence.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>
#include <QDate>
#include <QStringConverter>

namespace app_persistence {

namespace {

QString defaultUserFilePath() {
    return getUserSettingsFilePath();
}

QString defaultExpenseFilePath() {
    return getExpenseSettingsFilePath();
}

QString defaultRecurringFilePath() {
    return getRecurringExpensesFilePath();
}

} // namespace

QString getAppDataDirectoryPath() {
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dataDir.isEmpty()) {
        return QString();
    }
    QDir dir(dataDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return dir.absolutePath();
}

QString getUserSettingsFilePath() {
    const QString dataDir = getAppDataDirectoryPath();
    return dataDir.isEmpty() ? QString() : QDir(dataDir).filePath("users.txt");
}

QString getExpenseSettingsFilePath() {
    const QString dataDir = getAppDataDirectoryPath();
    return dataDir.isEmpty() ? QString() : QDir(dataDir).filePath("expenses.txt");
}

QString getRecurringExpensesFilePath() {
    const QString dataDir = getAppDataDirectoryPath();
    return dataDir.isEmpty() ? QString() : QDir(dataDir).filePath("recurring.txt");
}

bool saveUsers(const UserList& userList, const QString& filePath) {
    const QString targetPath = filePath.isEmpty() ? defaultUserFilePath() : filePath;
    if (targetPath.isEmpty()) {
        return false;
    }

    QFile file(targetPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out << userList.size() << "\n";
    for (int i = 1; i <= userList.size(); ++i) {
        const User user = userList.getUser(i);
        out << QString::fromStdString(user.getName()) << "|"
            << user.getSalary() << "|"
            << QString::fromStdString(user.getCardNumber()) << "\n";
    }
    file.close();
    return true;
}

bool loadUsers(UserList& userList, const QString& filePath) {
    const QString targetPath = filePath.isEmpty() ? defaultUserFilePath() : filePath;
    if (targetPath.isEmpty()) {
        return false;
    }

    QFile file(targetPath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream in(&file);
    const QString countLine = in.readLine();
    bool ok = false;
    const int userCount = countLine.toInt(&ok);
    if (!ok) {
        file.close();
        return false;
    }

    userList.clearUsers();
    for (int i = 0; i < userCount; ++i) {
        const QString line = in.readLine();
        if (line.isEmpty()) {
            continue;
        }
        const QStringList parts = line.split('|');
        if (parts.size() >= 2) {
            User user;
            user.setName(parts[0].toStdString());
            user.setSalary(parts[1].toInt());
            if (parts.size() >= 3) {
                user.setCardNumber(parts[2].toStdString());
            }
            userList.addUser(user);
        }
    }
    file.close();
    return true;
}

bool saveExpenses(const ExpenseList& expenseList, const QString& filePath) {
    const QString targetPath = filePath.isEmpty() ? defaultExpenseFilePath() : filePath;
    if (targetPath.isEmpty()) {
        return false;
    }

    QFile file(targetPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out << expenseList.size() << "\n";
    for (int i = 1; i <= expenseList.size(); ++i) {
        const Expense expense = expenseList.getExpense(i);
        out << QString::fromStdString(expense.getItem()) << "|"
            << expense.getAmount() << "|"
            << QString::fromStdString(expense.getPaidBy().getName()) << "|"
            << (expense.isEqualSplit() ? 1 : 0) << "|"
            << QString::fromStdString(expense.getDate()) << "|"
            << QString::fromStdString(expense.getStatementMonth()) << "|"
            << QString::fromStdString(expense.getPaidFor().empty() ? "Both" : expense.getPaidFor()) << "|"
            << QString::fromStdString(expense.getCardholder()) << "\n";
    }
    file.close();
    return true;
}

bool loadExpenses(
    ExpenseList& expenseList,
    UserList& userList,
    const QString& filePath,
    int selectedFilterYear,
    int selectedFilterMonth,
    const std::function<QDate(const QString&)>& parseDateFunc
) {
    const QString targetPath = filePath.isEmpty() ? defaultExpenseFilePath() : filePath;
    if (targetPath.isEmpty()) {
        return false;
    }

    QFile file(targetPath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        expenseList.clearExpenses();
        return false;
    }

    expenseList.clearExpenses();
    QTextStream in(&file);
    bool ok = false;
    const int expenseCount = in.readLine().toInt(&ok);
    if (!ok) {
        file.close();
        return false;
    }

    for (int i = 0; i < expenseCount; ++i) {
        const QString line = in.readLine();
        if (line.isEmpty()) {
            continue;
        }

        const QStringList parts = line.split('|');
        if (parts.size() < 4) {
            continue;
        }

        Expense expense;
        expense.setItem(parts[0].toStdString());
        expense.setAmount(parts[1].toDouble());
        expense.setEqualSplit(parts[3] == "1");

        const QString dateText = parts.value(4).trimmed();
        const QString statementMonthText = parts.value(5).trimmed();
        const QString paidForText = parts.value(6).trimmed();
        const QString cardholderText = parts.value(7).trimmed();
        if (dateText.isEmpty()) {
            expense.setDate(QDate::currentDate().toString(Qt::ISODate).toStdString());
        } else {
            const QDate parsed = parseDateFunc ? parseDateFunc(dateText) : QDate::fromString(dateText, Qt::ISODate);
            if (parsed.isValid()) {
                expense.setDate(parsed.toString(Qt::ISODate).toStdString());
            } else {
                expense.setDate(QDate::currentDate().toString(Qt::ISODate).toStdString());
            }
        }
        expense.setStatementMonth(statementMonthText.isEmpty() ? QString("%1-%2").arg(selectedFilterYear, 4, 10, QLatin1Char('0')).arg(selectedFilterMonth, 2, 10, QLatin1Char('0')).toStdString() : statementMonthText.toStdString());
        expense.setPaidFor(paidForText.isEmpty() ? "Both" : paidForText.toStdString());
        expense.setCardholder(cardholderText.isEmpty() ? std::string() : cardholderText.toStdString());

        User payer("Unknown", 0);
        const QString payerName = parts.value(2).trimmed();
        for (int j = 1; j <= userList.size(); ++j) {
            const User candidate = userList.getUser(j);
            if (candidate.getName() == payerName.toStdString()) {
                payer = candidate;
                break;
            }
        }
        expense.setPaidBy(payer);
        expenseList.addExpense(expense);
    }

    file.close();
    return true;
}

bool saveRecurringExpenses(const ExpenseList& recurringExpenses, const QString& filePath) {
    const QString targetPath = filePath.isEmpty() ? defaultRecurringFilePath() : filePath;
    if (targetPath.isEmpty()) {
        return false;
    }

    QFile file(targetPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out << "RECURRING_V1\n";
    out << recurringExpenses.size() << "\n";
    for (int i = 1; i <= recurringExpenses.size(); ++i) {
        const Expense e = recurringExpenses.getExpense(i);
        out << QString::fromStdString(e.getItem()) << "|"
            << e.getAmount() << "|"
            << (e.isEqualSplit() ? 1 : 0) << "|"
            << QString::fromStdString(e.getPaidFor().empty() ? "Both" : e.getPaidFor()) << "|"
            << QString::fromStdString(e.getPaidBy().getName()) << "\n";
    }
    file.close();
    return true;
}

bool loadRecurringExpenses(ExpenseList& recurringExpenses, const QString& filePath) {
    const QString targetPath = filePath.isEmpty() ? defaultRecurringFilePath() : filePath;
    if (targetPath.isEmpty()) {
        return false;
    }

    QFile file(targetPath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        recurringExpenses.clearExpenses();
        return false;
    }

    recurringExpenses.clearExpenses();
    QTextStream in(&file);
    const QString header = in.readLine();
    if (header != "RECURRING_V1") {
        file.close();
        return false;
    }

    bool ok = false;
    const int count = in.readLine().toInt(&ok);
    if (!ok) {
        file.close();
        return false;
    }

    for (int i = 0; i < count; ++i) {
        const QString line = in.readLine();
        if (line.isEmpty()) {
            continue;
        }
        const QStringList parts = line.split('|');
        if (parts.size() < 5) {
            continue;
        }

        Expense e;
        e.setItem(parts[0].toStdString());
        e.setAmount(parts[1].toDouble());
        e.setEqualSplit(parts[2] == "1");
        e.setPaidFor(parts[3].toStdString());
        User payer;
        payer.setName(parts[4].toStdString());
        e.setPaidBy(payer);
        recurringExpenses.addExpense(e);
    }

    file.close();
    return true;
}

} // namespace app_persistence
