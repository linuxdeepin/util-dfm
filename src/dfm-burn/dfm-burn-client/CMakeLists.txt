cmake_minimum_required(VERSION 3.10)

project(dfm${DFM_VERSION_MAJOR}-burn-client)

set(SRCS
    main.cpp
)

find_package(Qt${QT_VERSION_MAJOR}Core REQUIRED)

add_executable(${PROJECT_NAME} ${SRCS})

include_directories(
    ${PROJECT_SOURCE_DIR}/../dfm-burn-lib/inlcude
)

target_link_libraries(
    ${PROJECT_NAME}
    Qt${QT_VERSION_MAJOR}::Core
    dfm${DFM_VERSION_MAJOR}-burn
)
