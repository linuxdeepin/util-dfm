cmake_minimum_required(VERSION 3.1.0)

project(util-dfm)

# Setup the environment
SET(CMAKE_CXX_STANDARD 17)

SET(CMAKE_AUTOMOC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)
#SET(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")

option(OPT_ENABLE_QT6 "Use Qt6" ON)
set(DFM_BUILD_WITH_QT6 FALSE)

include(GNUInstallDirs)

if(OPT_ENABLE_QT6)
    find_package(Qt6 QUIET)
    if (Qt6_FOUND)
        message(STATUS "Found Qt6: ${Qt6_VERSION}")
        set(DFM_BUILD_WITH_QT6 TRUE)
        set(QT_VERSION_MAJOR 6)
        set(DFM_VERSION_MAJOR 6)
    endif()
endif()

if(NOT DFM_BUILD_WITH_QT6)
   message(WARNING "Qt6 not found, Use Qt5.")
   set(QT_VERSION_MAJOR 5)
   set(DFM_VERSION_MAJOR "")
endif()

# default

find_package(PkgConfig REQUIRED)

# Install settings
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX /usr)
endif ()

add_subdirectory(${PROJECT_SOURCE_DIR}/src)
