# Unit tests for dfm-io
if(DFM_BUILD_WITH_QT6)
    set(IO_TEST_LIB dfm6-io)
    set(QT_TEST_LIB Qt6::Test)
else()
    set(IO_TEST_LIB dfm-io)
    set(QT_TEST_LIB Qt5::Test)
endif()

message(STATUS "Adding unit tests for ${IO_TEST_LIB}")

# Collect test source files
file(GLOB_RECURSE TEST_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

# Create the test executable
add_executable(dfm-io-test
    ${TEST_SRCS}
)

# Link against the dfm-io library and Qt Test
target_link_libraries(dfm-io-test
    ${IO_TEST_LIB}
    ${QT_TEST_LIB}
)

# Add include directories
if(DFM_BUILD_WITH_QT6)
    target_include_directories(dfm-io-test
        PRIVATE
        ${CMAKE_SOURCE_DIR}/src/dfm-io/dfm-io
    )
else()
    target_include_directories(dfm-io-test
        PRIVATE
        ${CMAKE_SOURCE_DIR}/src/dfm-io/dfm-io
    )
endif()

# Register the test with CTest
add_test(NAME dfm-io-test COMMAND dfm-io-test)
