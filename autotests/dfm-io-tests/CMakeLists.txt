# Unit tests for dfm-io
if(DFM_BUILD_WITH_QT6)
    set(IO_TEST_LIB dfm6-io)
    set(QT_TEST_LIB Qt6::Test)
else()
    set(IO_TEST_LIB dfm-io)
    set(QT_TEST_LIB Qt5::Test)
endif()

message(STATUS "Adding unit tests for ${IO_TEST_LIB}")

# 共享的测试入口点
set(TEST_COMMON_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/test_main.cpp
)

# 各测试类源文件（不含 main）
set(TEST_CLASS_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/tst_dfm_io.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tst_dfilesorter.cpp
)

# Create the test executable
add_executable(dfm-io-test
    ${TEST_COMMON_SRCS}
    ${TEST_CLASS_SRCS}
)

# Link against the dfm-io library and Qt Test
target_link_libraries(dfm-io-test
    ${IO_TEST_LIB}
    ${QT_TEST_LIB}
)

# Add include directories
target_include_directories(dfm-io-test
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src/dfm-io/dfm-io
    ${CMAKE_SOURCE_DIR}/src/dfm-io/dfm-io/sort
)

# Register the test with CTest
add_test(NAME dfm-io-test COMMAND dfm-io-test)
