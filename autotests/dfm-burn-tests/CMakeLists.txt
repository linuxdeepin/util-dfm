# Unit tests for dfm-burn
if(DFM_BUILD_WITH_QT6)
    set(BURN_TEST_LIB dfm6-burn)
    set(QT_TEST_LIB Qt6::Test)
else()
    set(BURN_TEST_LIB dfm-burn)
    set(QT_TEST_LIB Qt5::Test)
endif()

message(STATUS "Adding unit tests for ${BURN_TEST_LIB}")

# Collect test source files
file(GLOB_RECURSE TEST_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

# Create the test executable
add_executable(dfm-burn-test
    ${TEST_SRCS}
)

# Link against the dfm-burn library and Qt Test
target_link_libraries(dfm-burn-test
    ${BURN_TEST_LIB}
    ${QT_TEST_LIB}
)

# Add include directories
target_include_directories(dfm-burn-test
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src/dfm-burn
)

# Register the test with CTest
add_test(NAME dfm-burn-test COMMAND dfm-burn-test)
