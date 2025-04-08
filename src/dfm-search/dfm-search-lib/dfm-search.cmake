# Setup the environment
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)
find_package(Dtk${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(Lucene REQUIRED IMPORTED_TARGET liblucene++ liblucene++-contrib)

# Build
add_library(${BIN_NAME} SHARED
            ${PUBLIC_INCLUDES}
            ${SRCS}
)

target_link_libraries(${BIN_NAME}
    Qt${QT_VERSION_MAJOR}::Core
    Dtk${QT_VERSION_MAJOR}::Core
    PkgConfig::Lucene
)
target_include_directories(
    ${BIN_NAME}
PUBLIC
    ${PROJECT_SOURCE_DIR}/include/${BASE_NAME}  
    ${PROJECT_SOURCE_DIR}/3rdparty/fulltext
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/fulltext
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

# Install
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
