# Setup the environment
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core  Concurrent REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GLIB glib-2.0 gobject-2.0 gio-2.0)
pkg_check_modules(mediainfoVal REQUIRED libmediainfo IMPORTED_TARGET)
list(APPEND mediainfos ${mediainfoVal_LDFLAGS})

# Build
add_library(${BIN_NAME} SHARED
            ${IO_PUBLIC_INCLUDES}
            ${IO_SRCS}
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

include(GNUInstallDirs)

# install lib
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
