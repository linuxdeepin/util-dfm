# Unit tests for dfm-search
if(DFM_BUILD_WITH_QT6)
    set(SEARCH_TEST_LIB dfm6-search)
    set(QT_TEST_LIB Qt6::Test)
else()
    set(SEARCH_TEST_LIB dfm-search)
    set(QT_TEST_LIB Qt5::Test)
endif()

message(STATUS "Adding unit tests for ${SEARCH_TEST_LIB}")

# Collect all test source files (including main.cpp)
file(GLOB TEST_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

# Create a single test executable
add_executable(dfm-search-test
    ${TEST_SRCS}
)

target_link_libraries(dfm-search-test
    ${SEARCH_TEST_LIB}
    ${QT_TEST_LIB}
)

target_include_directories(dfm-search-test
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src/dfm-search
    ${CMAKE_SOURCE_DIR}/src/dfm-search/dfm-search-lib
)

# Register the test with CTest
add_test(NAME dfm-search-test COMMAND dfm-search-test)
