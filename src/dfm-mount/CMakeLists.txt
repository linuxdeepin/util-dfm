cmake_minimum_required(VERSION 3.0)

set(BIN_NAME dfm-mount)

# Version
if (NOT PROJECT_VERSION)
    set(PROJECT_VERSION "1.0.0")
endif()

if (NOT PROJECT_VERSION_MAJOR)
    set(PROJECT_VERSION_MAJOR 0)
endif()

# signals and slots keyword is disable, use Q_SIGNALS and Q_SLOTS instead
add_compile_definitions(QT_NO_SIGNALS_SLOTS_KEYWORDS)

# gio signals conflicts with qt signals
add_definitions(-DQT_NO_KEYWORDS)

find_package(PkgConfig REQUIRED)
find_package(Qt5 COMPONENTS Core REQUIRED)
find_package(Qt5 COMPONENTS Concurrent REQUIRED)
find_package(Qt5 COMPONENTS DBus REQUIRED)
find_package(Qt5 COMPONENTS Widgets REQUIRED)

pkg_check_modules(udisks2 REQUIRED udisks2 IMPORTED_TARGET)
pkg_check_modules(glib REQUIRED glib-2.0 IMPORTED_TARGET)
pkg_check_modules(gio REQUIRED gio-unix-2.0 IMPORTED_TARGET)
pkg_check_modules(secret REQUIRED libsecret-1 IMPORTED_TARGET)
pkg_check_modules(mount REQUIRED mount IMPORTED_TARGET)

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

set_target_properties(
    dfm-mount PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_link_libraries(${BIN_NAME}
    Qt5::Core
    Qt5::Concurrent
    Qt5::DBus
    Qt5::Widgets
    PkgConfig::udisks2
    PkgConfig::glib
    PkgConfig::gio
    PkgConfig::secret
    PkgConfig::mount
)

target_include_directories(
    ${BIN_NAME}
PUBLIC
    ${PROJECT_SOURCE_DIR}/include/${BIN_NAME}
)

include(GNUInstallDirs)

# Install lib
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

# Build example
set(EXAMPLE_SRCS
    example/main.cpp)
add_executable(${BIN_NAME}_example ${EXAMPLE_SRCS})
target_link_libraries(${BIN_NAME}_example ${BIN_NAME})
target_link_libraries(${BIN_NAME}_example Qt5::Widgets)

#set(CMAKE_CXX_FLAGS "-fsanitize=undefined,address,leak -fno-omit-frame-pointer")
#set(CMAKE_C_FLAGS "-fsanitize=undefined,address,leak -fno-omit-frame-pointer")
#set(CMAKE_Ｌ_FLAGS "-fsanitize=undefined,address,leak -fno-omit-frame-pointer")

#set(CMAKE_CXX_FLAGS "-fsanitize=thread")
#set(CMAKE_C_FLAGS "-fsanitize=thread")
#set(CMAKE_Ｌ_FLAGS "-fsanitize=thread")

