
find_package(PkgConfig REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Concurrent REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS DBus REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

pkg_check_modules(udisks2 REQUIRED udisks2 IMPORTED_TARGET)
pkg_check_modules(glib REQUIRED glib-2.0 IMPORTED_TARGET)
pkg_check_modules(gio REQUIRED gio-unix-2.0 IMPORTED_TARGET)
pkg_check_modules(secret REQUIRED libsecret-1 IMPORTED_TARGET)
pkg_check_modules(mount REQUIRED mount IMPORTED_TARGET)

# Build
add_library(${BIN_NAME} SHARED
            ${PUBLIC_INCLUDES}
            ${SRCS}
)

if (NOT VERSION)
    set(VERSION "1.0.0")
endif()

if (NOT PROJECT_VERSION_MAJOR)
    set(PROJECT_VERSION_MAJOR 1)
endif()

set_target_properties(
    ${BIN_NAME} PROPERTIES
    VERSION ${VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_link_libraries(${BIN_NAME}
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::DBus
    Qt${QT_VERSION_MAJOR}::Widgets
    PkgConfig::udisks2
    PkgConfig::glib
    PkgConfig::gio
    PkgConfig::secret
    PkgConfig::mount
)

target_include_directories(
    ${BIN_NAME}
PUBLIC
    ${PROJECT_SOURCE_DIR}/include/${BASE_NAME}
)

include(GNUInstallDirs)

# Install lib
install(TARGETS ${BIN_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# install headers
install(DIRECTORY
    ${PROJECT_SOURCE_DIR}/include/${BASE_NAME}/${BASE_NAME}
    DESTINATION include/${BIN_NAME}
    FILES_MATCHING PATTERN "*.h"
)

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

# Build example
set(EXAMPLE_SRCS
    example/main.cpp)
add_executable(${BIN_NAME}_example ${EXAMPLE_SRCS})
target_link_libraries(${BIN_NAME}_example ${BIN_NAME})
target_link_libraries(${BIN_NAME}_example Qt${QT_VERSION_MAJOR}::Widgets)
