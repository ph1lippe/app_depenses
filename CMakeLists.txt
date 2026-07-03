cmake_minimum_required(VERSION 3.10.0)
project(app_depenses VERSION 0.1.0 LANGUAGES C CXX)

add_executable(app_depenses
src/expense.cpp
src/expenseList.cpp
src/main.cpp
src/user.cpp
src/userList.cpp
src/expenseManager.cpp
)

set(HEADER_FILES
src/expense.h
src/expenseList.h
src/user.h
src/expenseManager.h
src/userList.h)

include(CTest)
enable_testing()

