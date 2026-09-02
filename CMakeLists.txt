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

add_executable(app_depenses WIN32
src/domain/expense.cpp
src/domain/expenseList.cpp
src/domain/user.cpp
src/domain/userList.cpp
src/domain/expenseManager.cpp
src/domain/settlement.cpp
src/persistence/appdata_persistence.cpp
src/ui/main.cpp
src/ui/mainwindow.cpp
src/ui/MonthFilterWidget.cpp
src/ui/ExpenseEditorWidget.cpp
src/ui/ExpenseTableWidget.cpp
src/ui/SettlementResultWidget.cpp
src/ui/UserSettingsDialog.cpp
src/ui/RecurringExpensesDialog.cpp
)

target_link_libraries(app_depenses PRIVATE Qt6::Widgets)
target_include_directories(app_depenses PRIVATE src src/ui src/domain src/persistence)
install(TARGETS app_depenses RUNTIME DESTINATION .)

add_executable(test_settlement
src/domain/expense.cpp
src/domain/expenseList.cpp
src/domain/user.cpp
src/domain/userList.cpp
src/domain/settlement.cpp
tests/test_settlement.cpp
)

target_include_directories(test_settlement PRIVATE src src/domain)

target_link_libraries(test_settlement PRIVATE Qt6::Widgets)

add_test(NAME SettlementTest COMMAND test_settlement)

add_executable(test_equal_split_different_salaries
src/domain/expense.cpp
src/domain/expenseList.cpp
src/domain/user.cpp
src/domain/userList.cpp
src/domain/settlement.cpp
tests/test_equal_split_different_salaries.cpp
)

target_include_directories(test_equal_split_different_salaries PRIVATE src src/domain)

target_link_libraries(test_equal_split_different_salaries PRIVATE Qt6::Widgets)

add_test(NAME EqualSplitDifferentSalariesTest COMMAND test_equal_split_different_salaries)

add_executable(test_weighted_split
src/domain/expense.cpp
src/domain/expenseList.cpp
src/domain/user.cpp
src/domain/userList.cpp
src/domain/settlement.cpp
tests/test_weighted_split.cpp
)

target_include_directories(test_weighted_split PRIVATE src src/domain)

target_link_libraries(test_weighted_split PRIVATE Qt6::Widgets)

add_test(NAME WeightedSplitTest COMMAND test_weighted_split)

add_executable(test_negative_amount_editing
src/ui/ExpenseEditorWidget.cpp
src/ui/ExpenseTableWidget.cpp
src/ui/MonthFilterWidget.cpp
src/ui/SettlementResultWidget.cpp
src/ui/UserSettingsDialog.cpp
src/ui/RecurringExpensesDialog.cpp
src/domain/expense.cpp
src/domain/expenseList.cpp
src/domain/user.cpp
src/domain/userList.cpp
src/domain/expenseManager.cpp
src/domain/settlement.cpp
src/persistence/appdata_persistence.cpp
tests/test_negative_amount_editing.cpp
)

target_include_directories(test_negative_amount_editing PRIVATE src src/domain src/ui src/persistence)

target_link_libraries(test_negative_amount_editing PRIVATE Qt6::Widgets)

add_test(NAME NegativeAmountEditingTest COMMAND test_negative_amount_editing)

add_executable(test_expense_editor_validation
src/ui/ExpenseEditorWidget.cpp
src/ui/ExpenseTableWidget.cpp
src/ui/MonthFilterWidget.cpp
src/ui/SettlementResultWidget.cpp
src/ui/UserSettingsDialog.cpp
src/ui/RecurringExpensesDialog.cpp
src/domain/expense.cpp
src/domain/expenseList.cpp
src/domain/user.cpp
src/domain/userList.cpp
src/domain/expenseManager.cpp
src/domain/settlement.cpp
src/persistence/appdata_persistence.cpp
tests/test_expense_editor_validation.cpp
)

target_include_directories(test_expense_editor_validation PRIVATE src src/domain src/ui src/persistence)

target_link_libraries(test_expense_editor_validation PRIVATE Qt6::Widgets)

add_test(NAME ExpenseEditorValidationTest COMMAND test_expense_editor_validation)

add_executable(test_month_notes_persistence
src/domain/expense.cpp
src/domain/expenseList.cpp
src/domain/user.cpp
src/domain/userList.cpp
src/domain/expenseManager.cpp
src/domain/settlement.cpp
src/persistence/appdata_persistence.cpp
tests/test_month_notes_persistence.cpp
)

target_include_directories(test_month_notes_persistence PRIVATE src src/domain src/ui src/persistence)

target_link_libraries(test_month_notes_persistence PRIVATE Qt6::Widgets)

add_test(NAME MonthNotesPersistenceTest COMMAND test_month_notes_persistence)

add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_settlement test_equal_split_different_salaries test_weighted_split test_negative_amount_editing test_expense_editor_validation test_month_notes_persistence
)

add_custom_target(release_artifact
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target package
    COMMENT "Build and package the app as a release artifact"
    DEPENDS app_depenses
)

set(HEADER_FILES
src/domain/expense.h
src/domain/expenseList.h
src/domain/user.h
src/domain/expenseManager.h
src/domain/userList.h
src/ui/mainwindow.h)

include(CTest)
enable_testing()

# CPack packaging for release assets
set(CPACK_PACKAGE_NAME "app_depenses")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_CONTACT "")
set(CPACK_PACKAGE_VENDOR "")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Expense tracking application")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}")
set(CPACK_GENERATOR "ZIP")
include(CPack)

message(STATUS "CMake location: ${CMAKE_COMMAND}")