# Setup the environment
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)
find_package(Dtk${DFM_VERSION_MAJOR} COMPONENTS Core REQUIRED)
find_package(Threads REQUIRED)
find_package(Boost REQUIRED)

if (Boost_VERSION_STRING VERSION_LESS "1.89.0")
    message(STATUS "Boost < 1.89 → using Boost::system")
    find_package(Boost REQUIRED COMPONENTS system)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(Lucene REQUIRED IMPORTED_TARGET liblucene++ liblucene++-contrib)

# Build
add_library(${BIN_NAME} SHARED
            ${PUBLIC_INCLUDES}
            ${SRCS}
)

target_link_libraries(${BIN_NAME} PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Dtk${DFM_VERSION_MAJOR}::Core
    PkgConfig::Lucene
    Threads::Threads
)

if (Boost_VERSION_STRING VERSION_LESS "1.89.0")
    target_link_libraries(${BIN_NAME} PUBLIC Boost::system)
endif()

target_include_directories(
    ${BIN_NAME}
PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/${BASE_NAME}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/3rdparty/fulltext>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/fulltext>
    $<INSTALL_INTERFACE:include/${BIN_NAME}>
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
    EXPORT_NAME ${BIN_NAME}
)

# Install with export
install(TARGETS ${BIN_NAME} 
    EXPORT ${BIN_NAME}Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION include/${BIN_NAME}
)

# Install the export
install(EXPORT ${BIN_NAME}Targets
    FILE ${BIN_NAME}Targets.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${BIN_NAME}
)

# install headers
install(DIRECTORY
    ${PROJECT_SOURCE_DIR}/include/${BASE_NAME}/
    DESTINATION include/${BIN_NAME}
    FILES_MATCHING PATTERN "*.h"
)

# for pc file config - update to include all dependencies
set(PC_LIBS_PRIVATE Qt${QT_VERSION_MAJOR}Core dtk${DFM_VERSION_MAJOR}core)
set(PC_REQ_PRIVATE liblucene++ liblucene++-contrib)
set(PC_REQ_PUBLIC Qt${QT_VERSION_MAJOR}Core dtk${DFM_VERSION_MAJOR}core)

# config pkgconfig file
configure_file(${PROJECT_SOURCE_DIR}/misc/${BASE_NAME}/${BASE_NAME}.pc.in ${BIN_NAME}.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${BIN_NAME}.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

# config cmake file
configure_file(${PROJECT_SOURCE_DIR}/misc/${BASE_NAME}/${BASE_NAME}Config.cmake.in ${BIN_NAME}Config.cmake @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${BIN_NAME}Config.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${BIN_NAME}) 
