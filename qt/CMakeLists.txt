cmake_minimum_required(VERSION 3.22)
project(minesweeper-qt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(minesweeper-qt WIN32
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/boardwidget.cpp
    src/boardwidget.h
    src/customdialog.cpp
    src/customdialog.h
    src/game.cpp
    src/game.h
)

target_link_libraries(minesweeper-qt PRIVATE Qt6::Widgets)
