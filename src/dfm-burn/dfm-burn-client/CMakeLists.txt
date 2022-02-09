cmake_minimum_required(VERSION 3.1.0)

project(dfm-burn-client)

set(SRCS
    main.cpp
)

find_package(Qt5Core REQUIRED)

add_executable(${PROJECT_NAME} ${SRCS})

include_directories(
    ${PROJECT_SOURCE_DIR}/../dfm-burn-lib/inlcude
)

target_link_libraries(
    ${PROJECT_NAME}
    Qt5::Core
    dfm-burn
)
