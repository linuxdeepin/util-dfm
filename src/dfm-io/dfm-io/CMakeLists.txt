cmake_minimum_required(VERSION 3.1.0)

set(BIN_NAME dfm${DFM_VERSION_MAJOR}-io)
set(BASE_NAME dfm-io)

# Version
if (NOT PROJECT_VERSION)
    set(PROJECT_VERSION "1.0.0")
endif()

if (NOT PROJECT_VERSION_MAJOR)
    set(PROJECT_VERSION_MAJOR 0)
endif()

# Setup the environment
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Concurrent REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GLIB glib-2.0 gobject-2.0 gio-2.0)
pkg_check_modules(mediainfoVal REQUIRED libmediainfo IMPORTED_TARGET)
list(APPEND mediainfos ${mediainfoVal_LDFLAGS})

# gio signals conflicts with qt signals
add_definitions(-DQT_NO_KEYWORDS)

# public include
file(GLOB_RECURSE IO_PUBLIC_INCLUDES  CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/include/${BASE_NAME}/*")
# src
FILE (GLOB_RECURSE IO_SRCS CONFIGURE_DEPENDS
    "./*.c"
    "./*.cpp"
    "./*.h"
)

# Build
add_library(${BIN_NAME} SHARED
            ${IO_PUBLIC_INCLUDES}
            ${IO_SRCS}
)

set_target_properties(
    ${BIN_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_link_libraries(${BIN_NAME}
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Concurrent
    PkgConfig::mediainfoVal
    ${mediainfos}
    ${GLIB_LIBRARIES}
)

target_include_directories(${BIN_NAME}
PUBLIC
    ${GLIB_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/include/${BASE_NAME}
)

include(GNUInstallDirs)
install(DIRECTORY
    ${PROJECT_SOURCE_DIR}/include/${BASE_NAME}/${BASE_NAME}
    DESTINATION include/${BIN_NAME}
    FILES_MATCHING PATTERN "*.h"
)
install(TARGETS ${BIN_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# for pc file config
set(PC_LIBS_PRIVATE Qt${QT_VERSION_MAJOR}Core)
set(PC_REQ_PRIVATE)
set(PC_REQ_PUBLIC)

# config pkgconfig file
configure_file(${PROJECT_SOURCE_DIR}/misc/${BASE_NAME}/${BASE_NAME}.pc.in ${BIN_NAME}.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${BIN_NAME}.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

# config cmake file
configure_file(${PROJECT_SOURCE_DIR}/misc/${BASE_NAME}/${BASE_NAME}Config.cmake.in ${BIN_NAME}Config.cmake @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${BIN_NAME}Config.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${BIN_NAME})

