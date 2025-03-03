cmake_minimum_required(VERSION 3.1.0)

set (VERSION "1.0.0" CACHE STRING "define project version")
set(BASE_NAME dfm-burn)

# public include
file(GLOB_RECURSE PUBLIC_INCLUDES  CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/include/${BASE_NAME}/*")

# udfclient
FILE (GLOB_RECURSE UDFCLIENT_SRCS CONFIGURE_DEPENDS
    "../3rdparty/udfclient/*.c"
    "../3rdparty/udfclient/*.h"
)

# src
FILE (GLOB_RECURSE SRCS CONFIGURE_DEPENDS
    "./*.cpp"
    "./*.h"
)

#include(dfm-burn-qt5.cmake)

# Qt6
include(dfm-burn-qt6.cmake)

