cmake_minimum_required(VERSION 3.7)

project(util-dfm)

# Setup the environment
SET(CMAKE_CXX_STANDARD 17)

SET(CMAKE_AUTOMOC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)
#SET(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")


include(GNUInstallDirs)

find_package(Qt6 QUIET)
if (Qt6_FOUND)
    message(STATUS "Found Qt6: ${Qt6_VERSION}")
    set(QT_VERSION_MAJOR 6)
    set(DFM_VERSION_MAJOR 6)
else()
    message(WARNING "Qt6 not found!")
endif()

# default
find_package(PkgConfig REQUIRED)

# Install settings
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX /usr)
endif ()

add_subdirectory(${PROJECT_SOURCE_DIR}/src)
