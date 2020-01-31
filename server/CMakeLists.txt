cmake_minimum_required(VERSION 3.9)
project(DasServer)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/plugins)

set(PROJECT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

include_directories("${PROJECT_INCLUDE_DIR}")
include_directories("${PROJECT_SOURCE_DIR}")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)
find_package(Qt5Core)
find_package(Qt5Sql)
find_package(Qt5Network)
find_package(Qt5WebSockets)
find_package(Boost COMPONENTS system REQUIRED)

# ----
#[[

FIND_PATH(MYSQLPP_INCLUDE_PATH mysql++.h /usr/include/mysql++/)

FIND_PATH(MYSQL_INCLUDE_PATH mysql.h /usr/include/mysql/)
IF(MYSQL_INCLUDE_PATH AND MYSQLPP_INCLUDE_PATH)
  SET(MYSQLPP_INCLUDE_DIR
    ${MYSQLPP_INCLUDE_PATH}
    ${MYSQL_INCLUDE_PATH}
  )
ENDIF(MYSQL_INCLUDE_PATH AND MYSQLPP_INCLUDE_PATH)

FIND_LIBRARY(MYSQLPP_LIBRARIES mysqlpp)

IF (MYSQLPP_INCLUDE_DIR AND MYSQLPP_LIBRARIES)
   SET(MYSQLPP_FOUND TRUE)
ELSE (MYSQLPP_INCLUDE_DIR AND MYSQLPP_LIBRARIES)
   SET(MYSQLPP_FOUND FALSE)
ENDIF (MYSQLPP_INCLUDE_DIR AND MYSQLPP_LIBRARIES)

IF(MYSQLPP_FIND_REQUIRED)
  IF(MYSQLPP_FOUND)
  ELSE(MYSQLPP_FOUND)
    MESSAGE(FATAL_ERROR "Could not find mysqlpp")
  ENDIF(MYSQLPP_FOUND)
ENDIF(MYSQLPP_FIND_REQUIRED)


MARK_AS_ADVANCED(MYSQLPP_LIBRARIES MYSQLPP_INCLUDE_DIR)
]]
# ----

INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} "../helpz/include" "../lib" )

link_directories(
    "${CMAKE_CURRENT_BINARY_DIR}/.."
    "${CMAKE_CURRENT_BINARY_DIR}/../helpz"
    )

set( project_sources
   main.cpp
   webcommand.cpp

   dtls/dtls_tools.cpp
   dtls/dtls_credentials_manager.cpp
   dtls/dtls_server.cpp
   dtls/dtls_server_node.cpp
   dtls/dtls_server_thread.cpp

   protocol/protocol_base.cpp
   protocol/protocol_impl.cpp
   protocol/protocol_sender.cpp

   database/db_base.cpp
   database/db_proj.cpp
)

add_executable(${PROJECT_NAME} ${project_sources})

target_link_libraries(
    ${PROJECT_NAME}
    stdc++fs
    ${CMAKE_DL_LIBS}
    Qt5::Core
    Qt5::Sql
    Qt5::Network
    Qt5::WebSockets
    Threads::Threads
#    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    "mysqlclient"
#    "pqxx"
#    "pq"
    "botan-2"
    "HelpzDB"
    "Das"
    "DasPlus"
 )

#add_subdirectory(django)
#add_subdirectory(websocket)
#add_subdirectory(database)
#add_subdirectory(protocol)
