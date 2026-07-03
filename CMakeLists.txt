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
)

target_link_libraries(app_depenses PRIVATE Qt6::Widgets)
target_include_directories(app_depenses PRIVATE src)

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