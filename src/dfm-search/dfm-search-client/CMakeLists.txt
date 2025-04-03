cmake_minimum_required(VERSION 3.1.0)

project(dfm${DFM_VERSION_MAJOR}-search-client)

set(SRCS
    main.cpp
)

find_package(Qt${QT_VERSION_MAJOR}Core REQUIRED)

add_executable(${PROJECT_NAME} ${SRCS})

include_directories(
    ${PROJECT_SOURCE_DIR}/../dfm-search-lib/include
)

target_link_libraries(
    ${PROJECT_NAME}
    Qt${QT_VERSION_MAJOR}::Core
    dfm${DFM_VERSION_MAJOR}-search
) 
