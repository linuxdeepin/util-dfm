cmake_minimum_required(VERSION 3.1.0)

project(dfm-io-unitest)

set(dfm-io_tst_SRCS
    main.cpp
    ut_denumerator.cpp
)

# Setup the environment
find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Gui REQUIRED)

SET(CMAKE_AUTOMOC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)
#SET(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../utils/cpp-stub
    ${CMAKE_CURRENT_SOURCE_DIR}/../utils/stub-ext
    ${PROJECT_SOURCE_DIR}/../../src/dfm-io/libdfm-io/include
    ${PROJECT_SOURCE_DIR}/../../src/dfm-io/libdfm-io/private
)

# Build
set(BINARY ${PROJECT_NAME}_tst)

add_executable(${BINARY}
    ${dfm-io_tst_SRCS}
)

target_link_libraries(${BINARY}
    Qt5::Core
    Qt5::Widgets
    dfm-io
    gtest_main
    gtest
    gmock
    pthread
)

enable_testing()
