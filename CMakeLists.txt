cmake_minimum_required(VERSION 3.10.0)
project(app_depenses VERSION 0.1.0 LANGUAGES C CXX)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /permissive-)
endif()

add_executable(app_depenses
src/expense.cpp
src/expenseList.cpp
src/main.cpp
src/mainwindow.cpp
src/user.cpp
src/userList.cpp
src/expenseManager.cpp
src/settlement.cpp
)

target_link_libraries(app_depenses PRIVATE Qt6::Widgets)
target_include_directories(app_depenses PRIVATE src)

add_executable(test_settlement
src/expense.cpp
src/expenseList.cpp
src/user.cpp
src/userList.cpp
src/settlement.cpp
tests/test_settlement.cpp
)

target_include_directories(test_settlement PRIVATE src)

target_link_libraries(test_settlement PRIVATE Qt6::Widgets)

add_test(NAME SettlementTest COMMAND test_settlement)

add_executable(test_equal_split_different_salaries
src/expense.cpp
src/expenseList.cpp
src/user.cpp
src/userList.cpp
src/settlement.cpp
tests/test_equal_split_different_salaries.cpp
)

target_include_directories(test_equal_split_different_salaries PRIVATE src)

target_link_libraries(test_equal_split_different_salaries PRIVATE Qt6::Widgets)

add_test(NAME EqualSplitDifferentSalariesTest COMMAND test_equal_split_different_salaries)

add_executable(test_weighted_split
src/expense.cpp
src/expenseList.cpp
src/user.cpp
src/userList.cpp
src/settlement.cpp
tests/test_weighted_split.cpp
)

target_include_directories(test_weighted_split PRIVATE src)

target_link_libraries(test_weighted_split PRIVATE Qt6::Widgets)

add_test(NAME WeightedSplitTest COMMAND test_weighted_split)

set(HEADER_FILES
src/expense.h
src/expenseList.h
src/user.h
src/expenseManager.h
src/userList.h
src/mainwindow.h)

include(CTest)
enable_testing()

message(STATUS "CMake location: ${CMAKE_COMMAND}")