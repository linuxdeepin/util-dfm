cmake_minimum_required(VERSION 3.10)

project(dfm-searcher)

set(SRCS
    main.cpp
    cli_options.cpp
    cli_options.h
    time_parser.cpp
    time_parser.h
    output/output_formatter.h
    output/text_output.cpp
    output/text_output.h
    output/json_output.cpp
    output/json_output.h
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


install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
