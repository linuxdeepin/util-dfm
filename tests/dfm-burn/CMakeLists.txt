cmake_minimum_required(VERSION 3.1.0)

project(dfm-burn-unitest)

set(dfm-burn_tst_SRCS
    ut_opticaldiscinfo.cpp
)

# Setup the environment
find_package(Qt5Core REQUIRED)

SET(CMAKE_AUTOMOC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)
#SET(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build
set(BINARY ${PROJECT_NAME}_tst)
add_executable(${BINARY} ${dfm-burn_tst_SRCS})
target_link_libraries(${BINARY}
    Qt5::Core
    dfm-burn
    gtest_main
    gtest
    gmock
    pthread
)

enable_testing()
