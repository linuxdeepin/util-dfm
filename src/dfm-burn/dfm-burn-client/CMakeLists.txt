cmake_minimum_required(VERSION 3.10)

project(dfm-burner)

set(SRCS
    main.cpp
    cli_options.cpp
    cli_options.h
    burn_utils.cpp
    burn_utils.h
)

find_package(Qt${QT_VERSION_MAJOR}Core REQUIRED)

add_executable(${PROJECT_NAME} ${SRCS})

include_directories(
    ${PROJECT_SOURCE_DIR}/../../include
)

target_link_libraries(
    ${PROJECT_NAME}
    Qt${QT_VERSION_MAJOR}::Core
    dfm${DFM_VERSION_MAJOR}-burn
)

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
