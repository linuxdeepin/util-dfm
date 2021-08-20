cmake_minimum_required(VERSION 3.1.0)

project(util-dfm-tests)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DQT_DEBUG")

set(PROJECT_SOURCE_PATH "${CMAKE_SOURCE_DIR}/../src")
set(TEST_UTILS_PATH "${CMAKE_SOURCE_DIR}/utils")

# 用于测试覆盖率的编译条件
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-inline -fno-access-control -O0 -fprofile-arcs -ftest-coverage -lgcov")

# UT依赖
find_package(GTest REQUIRED)
find_package(Qt5 COMPONENTS Test REQUIRED)

# 打桩工具
FILE(GLOB CPP_STUB_SRC "${TEST_UTILS_PATH}/cpp-stub/*.h"
    "${TEST_UTILS_PATH}/cpp-stub/*.hpp"
    "${TEST_UTILS_PATH}/stub-ext/*.h"
    "${TEST_UTILS_PATH}/stub-ext/*.cpp")

include_directories("${TEST_UTILS_PATH}/cpp-stub" "${TEST_UTILS_PATH}/stub-ext")
include_directories(${PROJECT_SOURCE_PATH})

add_subdirectory(${PROJECT_SOURCE_DIR}/dfm-burn)
add_subdirectory(${PROJECT_SOURCE_DIR}/dfm-io)

