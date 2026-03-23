# Unit tests for util-dfm using Qt Test
cmake_minimum_required(VERSION 3.10)

project(autotests)

# Enable/disable building of unit tests (default: ON)
option(BUILD_UNIT_TESTS "Build unit tests" ON)

if(NOT BUILD_UNIT_TESTS)
    return()
endif()

message(STATUS "Building unit tests")

# Find Qt Test component based on Qt version
if(QT_VERSION_MAJOR EQUAL 6)
    find_package(Qt6 REQUIRED COMPONENTS Test Core)
else()
    find_package(Qt5 REQUIRED COMPONENTS Test Core)
endif()

# Common test configuration
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DQT_DEBUG")

# Enable testing
enable_testing()

# Add test subdirs for each module
add_subdirectory(dfm-io-tests)
add_subdirectory(dfm-mount-tests)
add_subdirectory(dfm-burn-tests)
add_subdirectory(dfm-search-tests)

# Add top-level test target (run all tests)
add_custom_target(test-all
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS dfm-io-test dfm-mount-test dfm-burn-test dfm-search-test
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all unit tests..."
)
