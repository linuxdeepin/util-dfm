cmake_minimum_required(VERSION 3.1.0)

set (VERSION "1.0.0" CACHE STRING "define project version")
set(BASE_NAME dfm-io)

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

include(dfm-io-qt5.cmake)

# Qt6
if (DFM_BUILD_WITH_QT6)
include(dfm-io-qt6.cmake)
endif()


