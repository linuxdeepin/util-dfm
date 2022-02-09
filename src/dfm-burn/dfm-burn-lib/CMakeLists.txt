cmake_minimum_required(VERSION 3.1.0)

project(dfm-burn)

# Version
if (NOT PROJECT_VERSION)
    set(PROJECT_VERSION "1.0.0")
endif()

if (NOT PROJECT_VERSION_MAJOR)
    set(PROJECT_VERSION_MAJOR 0)
endif()

# Setup the environment
find_package(Qt5Core REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(DEPENDS REQUIRED libisoburn-1)
set(PkgConfigFindModules libisoburn-1)
foreach(PkgConfigModule ${PkgConfigFindModules})
    pkg_check_modules(PkgUseModuleVal REQUIRED ${PkgConfigModule})
    list(APPEND PkgUseModules ${PkgUseModuleVal_LDFLAGS})
    include_directories(${PkgUseModuleVal_INCLUDE_DIRS})
endforeach(PkgConfigModule)


# src
FILE(GLOB BURN_PUBLIC_HEADER_FILES
    "include/*.h")

FILE(GLOB BURN_PRIVATE_FILES
    "private/*.h"
    "private/*.hpp"
    "private/*.cpp"
    )

FILE(GLOB BURN_FILES
    "*.cpp"
    )


# Build
add_library(${PROJECT_NAME} SHARED
    ${BURN_PUBLIC_HEADER_FILES}
    ${BURN_PRIVATE_FILES}
    ${BURN_FILES}
    )

target_link_libraries(
    ${PROJECT_NAME}
    Qt5::Core
    ${PkgUseModules}
)

target_include_directories(
    ${PROJECT_NAME}
PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)


# Install
install (FILES ${BURN_PUBLIC_HEADER_FILES}
    DESTINATION include/dfm-burn
)

set_target_properties(
    dfm-burn PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)


install_targets ("/${CMAKE_INSTALL_LIBDIR}" dfm-burn)

configure_file ("dfm-burn.pc.in" "dfm-burn.pc" @ONLY)
install (FILES ${CMAKE_CURRENT_BINARY_DIR}/dfm-burn.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

