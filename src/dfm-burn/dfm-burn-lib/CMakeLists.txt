cmake_minimum_required(VERSION 3.10)

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

if (QT_VERSION_MAJOR MATCHES 6)
    include(dfm-burn-qt6.cmake)
else()
    include(dfm-burn-qt5.cmake)
endif()
