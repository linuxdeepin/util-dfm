cmake_minimum_required(VERSION 3.10)

set (VERSION "1.0.0" CACHE STRING "define project version")
set(BASE_NAME dfm-search)

# public include
file(GLOB_RECURSE PUBLIC_INCLUDES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/include/${BASE_NAME}/*")

# src
FILE (GLOB_RECURSE SRCS CONFIGURE_DEPENDS
    "./*.cpp"
    "./*.h"
    "../3rdparty/*.cpp"
    "../3rdparty/*.h"
)

# Qt6
if (QT_VERSION_MAJOR MATCHES 6)
    include(dfm-search-qt6.cmake)
else()
    include(dfm-search-qt5.cmake)
endif()





