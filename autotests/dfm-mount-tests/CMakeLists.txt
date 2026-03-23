# Unit tests for dfm-mount
if(DFM_BUILD_WITH_QT6)
    set(MOUNT_TEST_LIB dfm6-mount)
    set(QT_TEST_LIB Qt6::Test)
else()
    set(MOUNT_TEST_LIB dfm-mount)
    set(QT_TEST_LIB Qt5::Test)
endif()

message(STATUS "Adding unit tests for ${MOUNT_TEST_LIB}")

# Collect test source files
file(GLOB_RECURSE TEST_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

# Create the test executable
add_executable(dfm-mount-test
    ${TEST_SRCS}
)

# Link against the dfm-mount library and Qt Test
target_link_libraries(dfm-mount-test
    ${MOUNT_TEST_LIB}
    ${QT_TEST_LIB}
)

# Add include directories
target_include_directories(dfm-mount-test
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src/dfm-mount
)

# Register the test with CTest
add_test(NAME dfm-mount-test COMMAND dfm-mount-test)
