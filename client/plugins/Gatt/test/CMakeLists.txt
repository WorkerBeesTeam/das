cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(APP gatt_test)
project(${APP} VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Threads REQUIRED)
find_package(QT NAMES Qt5 COMPONENTS Core Network Sql REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Network Sql REQUIRED)

set(DAS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../..)

if(NOT DEFINED DAS_BUILD_PATH)
    set(DAS_BUILD_PATH ${DAS_DIR}/build/Desktop-Debug)
endif()

include_directories(
    ${DAS_BUILD_PATH}/helpz/include
    ${DAS_DIR}/lib
)

set(SRC test.cpp
    checker_test_base.h checker_test_base.cpp
    gatt_test.h gatt_test.cpp
)

add_executable(${APP} ${SRC})
add_definitions(-DPLUGINS_PATH="${DAS_BUILD_PATH}/plugins")

target_link_directories(${APP} PUBLIC
    ${DAS_BUILD_PATH}/helpz
    ${DAS_BUILD_PATH}
)
target_link_libraries(${APP}
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Sql
    Threads::Threads
    HelpzBase
    HelpzDBMeta
    Das
)
