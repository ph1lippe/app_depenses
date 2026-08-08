#ifndef APPDATA_PERSISTENCE_H
#define APPDATA_PERSISTENCE_H

#include <QDate>
#include <QString>
#include <functional>

#include "expenseList.h"
#include "userList.h"

namespace app_persistence {

QString getAppDataDirectoryPath();
QString getUserSettingsFilePath();
QString getExpenseSettingsFilePath();
QString getRecurringExpensesFilePath();

bool saveUsers(const UserList& userList, const QString& filePath = QString());
bool loadUsers(UserList& userList, const QString& filePath = QString());

bool saveExpenses(const ExpenseList& expenseList, const QString& filePath = QString());
bool loadExpenses(
    ExpenseList& expenseList,
    UserList& userList,
    const QString& filePath = QString(),
    int selectedFilterYear = 0,
    int selectedFilterMonth = 0,
    const std::function<QDate(const QString&)>& parseDateFunc = {}
);

bool saveRecurringExpenses(const ExpenseList& recurringExpenses, const QString& filePath = QString());
bool loadRecurringExpenses(ExpenseList& recurringExpenses, const QString& filePath = QString());

} // namespace app_persistence

#endif // APPDATA_PERSISTENCE_H
