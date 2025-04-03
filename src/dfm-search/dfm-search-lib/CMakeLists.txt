cmake_minimum_required(VERSION 3.1.0)

set (VERSION "1.0.0" CACHE STRING "define project version")
set(BASE_NAME dfm-search)

# public include
file(GLOB_RECURSE PUBLIC_INCLUDES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/include/${BASE_NAME}/*")

# src
FILE (GLOB_RECURSE SRCS CONFIGURE_DEPENDS
    "./*.cpp"
    "./*.h"
)

# Qt6
include(dfm-search-qt6.cmake)
