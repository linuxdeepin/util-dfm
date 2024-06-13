cmake_minimum_required(VERSION 3.1.0)

project(util-dfm)

# Setup the environment
SET(CMAKE_CXX_STANDARD 17)

SET(CMAKE_AUTOMOC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)
#SET(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")

option(OPT_ENABLE_QT6 "Use Qt6" OFF)

include(GNUInstallDirs)

if(OPT_ENABLE_QT6)
    find_package(QT NAMES Qt6 CONFIG REQUIRED Core)
    set(DFM_VERSION_MAJOR 6)
else()
    set(QT_MINIMUM_VERSION "5.6.3")
    find_package(QT NAMES Qt5 ${QT_MINIMUM_VERSION} CONFIG REQUIRED Core)
    set(DFM_VERSION_MAJOR "")
endif()

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
message("Using Qt version: ${QT_VERSION}")

find_package(PkgConfig REQUIRED)

# Install settings
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX /usr)
endif ()

add_subdirectory(${PROJECT_SOURCE_DIR}/src)
