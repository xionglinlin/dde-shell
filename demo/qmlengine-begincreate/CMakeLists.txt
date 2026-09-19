# SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: CC0-1.0
#
# 这是一个【独立】的教学 demo 工程，不参与 dde-shell 主构建。
# 单独构建方式见 README.md。
cmake_minimum_required(VERSION 3.16)

project(qmlengine_begincreate_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Qml Quick)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Qml Quick)

add_executable(qmlengine_begincreate_demo
    main.cpp
    MiniQmlEngine.h
    MiniQmlEngine.cpp
    demo.qrc
)

target_link_libraries(qmlengine_begincreate_demo PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Quick
)
