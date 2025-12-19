cmake_minimum_required(VERSION 3.10)

set(BASE_NAME dfm-mount)

# signals and slots keyword is disable, use Q_SIGNALS and Q_SLOTS instead
add_compile_definitions(QT_NO_SIGNALS_SLOTS_KEYWORDS)

# gio signals conflicts with qt signals
add_definitions(-DQT_NO_KEYWORDS)

# public include
file(GLOB_RECURSE PUBLIC_INCLUDES  CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/include/${BASE_NAME}/*")
# src
FILE (GLOB_RECURSE SRCS CONFIGURE_DEPENDS
    "./*.cpp"
    "./*.h"
)

if (QT_VERSION_MAJOR MATCHES 6)
    include(dfm-mount-qt6.cmake)
else()
    include(dfm-mount-qt5.cmake)
endif()

#set(CMAKE_CXX_FLAGS "-fsanitize=undefined,address,leak -fno-omit-frame-pointer")
#set(CMAKE_C_FLAGS "-fsanitize=undefined,address,leak -fno-omit-frame-pointer")
#set(CMAKE_Ｌ_FLAGS "-fsanitize=undefined,address,leak -fno-omit-frame-pointer")

#set(CMAKE_CXX_FLAGS "-fsanitize=thread")
#set(CMAKE_C_FLAGS "-fsanitize=thread")
#set(CMAKE_Ｌ_FLAGS "-fsanitize=thread")
