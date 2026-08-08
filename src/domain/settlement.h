#ifndef SETTLEMENT_H
#define SETTLEMENT_H

#include <string>
#include "expenseList.h"
#include "userList.h"

std::string computeSettlementResult(const UserList& userList, const ExpenseList& expenseList, int selectedYear, int selectedMonth);

#endif // SETTLEMENT_H
