cmake_minimum_required(VERSION 3.1.0)

project(util-dfm)

# Setup the environment
SET(CMAKE_CXX_STANDARD 17)

option(BUILD_UTILS "Build utilities" OFF)
SET(CMAKE_AUTOMOC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)
#SET(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")
set(QT_MINIMUM_VERSION "5.6.3")

include(GNUInstallDirs)
find_package(Qt5 ${QT_MINIMUM_VERSION} CONFIG REQUIRED Core)
find_package(PkgConfig REQUIRED)

# Install settings
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX /usr)
endif ()

add_subdirectory(${PROJECT_SOURCE_DIR}/src)
