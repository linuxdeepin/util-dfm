cmake_minimum_required(VERSION 3.1.0)

project(dfm-burn)

set(BURN_PUBLIC_HEADER_FILES
    include/dfm_burn.h
    include/dfm_burn_namespace.h
    include/opticaldiscinfo.h
    )

set(BURN_PRIVATE_HEADER_FILES
    private/opticaldiscinfo_p.h
    private/abstractopticaldiscengine_p.h
    private/xorrisoengine_p.h
    private/engine_factory.hpp
    )

set(BURN_CPP_FILES
    opticaldiscinfo.cpp
    private/abstractopticaldiscengine.cpp
    private/xorrisoengine.cpp
    )

# Setup the environment
find_package(Qt5Core REQUIRED)
find_package(Qt5DBus REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(UDisks2 udisks2-qt5) # look into FindPkgConfig.cmake

# Build
add_library(${PROJECT_NAME} SHARED
    ${BURN_PUBLIC_HEADER_FILES}
    ${BURN_PRIVATE_HEADER_FILES}
    ${BURN_CPP_FILES}
    )

target_link_libraries(
    ${PROJECT_NAME}
    Qt5::Core
    Qt5::DBus
    ${UDisks2_LIBRARIES}
)

target_include_directories(
    ${PROJECT_NAME}
PRIVATE
    ${UDisks2_INCLUDE_DIRS}
PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)


# Install
install (FILES ${BURN_PUBLIC_HEADER_FILES}
    DESTINATION include/dfm-burn
)

install_targets ("/${CMAKE_INSTALL_LIBDIR}" dfm-burn)
