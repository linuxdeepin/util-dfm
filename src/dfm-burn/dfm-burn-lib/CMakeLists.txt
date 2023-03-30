cmake_minimum_required(VERSION 3.1.0)

set(BIN_NAME dfm-burn)

# Version
if (NOT PROJECT_VERSION)
    set(PROJECT_VERSION "1.0.0")
endif()

if (NOT PROJECT_VERSION_MAJOR)
    set(PROJECT_VERSION_MAJOR 1)
endif()

# Setup the environment
find_package(Qt5 COMPONENTS Core REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(isoburn REQUIRED libisoburn-1 IMPORTED_TARGET)

# public include
file(GLOB_RECURSE PUBLIC_INCLUDES  CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/include/${BIN_NAME}/*")
# src
FILE (GLOB_RECURSE SRCS CONFIGURE_DEPENDS
    "./*.cpp"
    "./*.h"
)

# Build
add_library(${BIN_NAME} SHARED
            ${PUBLIC_INCLUDES}
            ${SRCS}
)

target_link_libraries(${BIN_NAME}
    Qt5::Core
    PkgConfig::isoburn
)

target_include_directories(
    ${BIN_NAME}
PUBLIC
    ${PROJECT_SOURCE_DIR}/include/${BIN_NAME}
)

set_target_properties(
    dfm-burn PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Install
install(DIRECTORY
    ${PROJECT_SOURCE_DIR}/include/${BIN_NAME}
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)
install(TARGETS ${BIN_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# for pc file config
set(PC_LIBS_PRIVATE Qt${QT_VERSION_MAJOR}Core)
set(PC_REQ_PRIVATE)
set(PC_REQ_PUBLIC)

# config pkgconfig file
configure_file(${PROJECT_SOURCE_DIR}/misc/${BIN_NAME}/${BIN_NAME}.pc.in ${BIN_NAME}.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${BIN_NAME}.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

# config cmake file
configure_file(${PROJECT_SOURCE_DIR}/misc/${BIN_NAME}/${BIN_NAME}Config.cmake.in ${BIN_NAME}Config.cmake @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${BIN_NAME}Config.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${BIN_NAME})


