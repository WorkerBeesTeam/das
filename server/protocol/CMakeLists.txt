cmake_minimum_required(VERSION 3.9)

project(protocol VERSION 1.0.3 DESCRIPTION "protocol description")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#include(GNUInstallDirs)
set( project_sources
    protocol_base.cpp
    protocol_impl.cpp
    protocol_sender.cpp
)

INCLUDE_DIRECTORIES( "../../helpz/include" "../../lib" "../" )

add_library(${PROJECT_NAME} SHARED ${project_sources})
set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER protocol_impl.h)
target_include_directories(${PROJECT_NAME} PRIVATE .)

find_package(Qt5Core)
find_package(Qt5Network)

target_link_libraries(
    ${PROJECT_NAME}
    Qt5::Core
    Qt5::Network
    "Das"
 )
