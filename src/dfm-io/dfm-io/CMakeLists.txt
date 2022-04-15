project(dfm-io)

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

pkg_check_modules(GLIB glib-2.0 gobject-2.0 gio-2.0)

pkg_check_modules(mediainfoVal REQUIRED libmediainfo IMPORTED_TARGET)
list(APPEND mediainfos ${mediainfoVal_LDFLAGS})

# gio signals conflicts with qt signals
add_definitions(-DQT_NO_KEYWORDS)

# include
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/inlcude
    ${CMAKE_CURRENT_SOURCE_DIR}/private
)

# public include
FILE (GLOB IO_PUBLIC_INCLUDES "include/*.h" "include/*.hpp"
    "include/core/*.h" "include/core/*.hpp"                 #core
    "include/error/*.h" "include/error/*.hpp"               #error
    "include/factory/*.h" "include/factory/*.hpp"           #factory
    "include/local/*.h" "include/local/*.hpp"               #local
)


# private include
FILE (GLOB IO_PRIVATE_INCLUDES "private/*.h" "private/*.hpp"
    "private/core/*.h" "private/core/*.hpp"                 #core
    "private/factory/*.h" "private/factory/*.hpp"           #factory
    "private/local/*.h" "private/local/*.hpp"               #local
    "utils/*.h"                                             #utils
)

# src
FILE (GLOB IO_SRCS "src/*.c" "src/*.cpp"
    "src/core/*.c" "src/core/*.cpp"                         #core
    "src/factory/*.c" "src/factory/*.cpp"                   #factory
    "src/local/*.c" "src/local/*.cpp"                       #local
    "utils/*.cpp"                                           #utils
)

# Build
add_library(${PROJECT_NAME} SHARED
            ${IO_PUBLIC_INCLUDES}
            ${IO_PRIVATE_INCLUDES}
            ${IO_SRCS}
)

set_target_properties(
    dfm-io PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_link_libraries(${PROJECT_NAME}
    Qt5::Core
    PkgConfig::mediainfoVal
    ${mediainfos}
    ${GLIB_LIBRARIES}
)

target_include_directories(${PROJECT_NAME}
PUBLIC
    ${GLIB_INCLUDE_DIRS}
    ${CMAKE_CURRENT_LIST_DIR}/private
    ${CMAKE_CURRENT_LIST_DIR}/include
)

qt5_use_modules(${PROJECT_NAME}
    Core
)

# Install
FILE (GLOB IO_INCLUDES_INSTALL
    "include/*.h" "include/*.hpp"
)
FILE (GLOB IO_INCLUDES_INSTALL_CORE
    "include/core/*.h" "include/core/*.hpp"
)
FILE (GLOB IO_INCLUDES_INSTALL_ERROR
    "include/error/*.h" "include/error/*.hpp"
)
FILE (GLOB IO_INCLUDES_INSTALL_FACTORY
    "include/factory/*.h" "include/factory/*.hpp"
)
FILE (GLOB IO_INCLUDES_INSTALL_LOCAL
    "include/local/*.h" "include/local/*.hpp"
)

# private
#FILE (GLOB IO_INCLUDES_INSTALL_CORE
#    "private/core/*.h" "private/core/*.hpp"
#)
#FILE (GLOB IO_INCLUDES_INSTALL_ERROR
#    "private/error/*.h" "private/error/*.hpp"
#)
#FILE (GLOB IO_INCLUDES_INSTALL_FACTORY
#    "private/factory/*.h" "private/factory/*.hpp"
#)
#FILE (GLOB IO_INCLUDES_INSTALL_LOCAL
#    "private/local/*.h" "private/local/*.hpp"
#)

include(GNUInstallDirs)

install (FILES ${IO_INCLUDES_INSTALL}
    DESTINATION include/dfm-io
)
install (FILES ${IO_INCLUDES_INSTALL_CORE}
    DESTINATION include/dfm-io/core
)
install (FILES ${IO_INCLUDES_INSTALL_ERROR}
    DESTINATION include/dfm-io/error
)
install (FILES ${IO_INCLUDES_INSTALL_FACTORY}
    DESTINATION include/dfm-io/factory
)
install (FILES ${IO_INCLUDES_INSTALL_LOCAL}
    DESTINATION include/dfm-io/local
)


# todo install private

install_targets ("/${CMAKE_INSTALL_LIBDIR}" dfm-io)


configure_file ("dfm-io.pc.in" "dfm-io.pc" @ONLY)
install (FILES ${CMAKE_CURRENT_BINARY_DIR}/dfm-io.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
