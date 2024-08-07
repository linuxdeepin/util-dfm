add_definitions(-DQT_NO_KEYWORDS)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -fsanitize=address")

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

if (QT_VERSION_MAJOR EQUAL "5")
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set (CMAKE_VERBOSE_MAKEFILE ON)
        message("debug type open sanitize check")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined,address,leak -fno-omit-frame-pointer")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined,address,leak -fno-omit-frame-pointer")
        set(CMAKE_L_FLAGS "${CMAKE_L_FLAGS} -fsanitize=undefined,address,leak -fno-omit-frame-pointer")

        #set(CMAKE_CXX_FLAGS "-fsanitize=thread")
    endif()
endif()


# include
include_directories(
    ${PROJECT_SOURCE_DIR}/../dfm-io/inlcude
)

add_executable(dfm-list dfm-list.cpp)
target_link_libraries(dfm-list dfm${DFM_VERSION_MAJOR}-io)

add_executable(dfm-sort-list dfm-sort-list.cpp)
target_link_libraries(dfm-sort-list dfm${DFM_VERSION_MAJOR}-io)

add_executable(dfm-info dfm-info.cpp)
target_link_libraries(dfm-info dfm${DFM_VERSION_MAJOR}-io)

add_executable(dfm-copy dfm-copy.cpp)
target_link_libraries(dfm-copy dfm${DFM_VERSION_MAJOR}-io)

add_executable(dfm-copy-gio dfm-copy-gio.cpp)
target_link_libraries(dfm-copy-gio dfm${DFM_VERSION_MAJOR}-io)

add_executable(dfm-copy3 dfm-copy3.cpp)
target_link_libraries(dfm-copy3 dfm${DFM_VERSION_MAJOR}-io)

add_executable(dfm-copy-system dfm-copy-system.cpp)
target_link_libraries(dfm-copy-system dfm${DFM_VERSION_MAJOR}-io)

add_executable(dfm-cat dfm-cat.cpp)
target_link_libraries(dfm-cat dfm${DFM_VERSION_MAJOR}-io)

add_executable(dfm-save dfm-save.cpp)
target_link_libraries(dfm-save dfm${DFM_VERSION_MAJOR}-io)

add_executable(dfm-rename dfm-rename.cpp)
target_link_libraries(dfm-rename dfm${DFM_VERSION_MAJOR}-io)

add_executable(dfm-move dfm-move.cpp)
target_link_libraries(dfm-move dfm${DFM_VERSION_MAJOR}-io)

add_executable(dfm-trash dfm-trash.cpp)
target_link_libraries(dfm-trash dfm${DFM_VERSION_MAJOR}-io)

add_executable(dfm-delete dfm-delete.cpp)
target_link_libraries(dfm-delete dfm${DFM_VERSION_MAJOR}-io)

add_executable(dfm-restore dfm-restore.cpp)
target_link_libraries(dfm-restore dfm${DFM_VERSION_MAJOR}-io)

add_executable(dfm-touch dfm-touch.cpp)
target_link_libraries(dfm-touch dfm${DFM_VERSION_MAJOR}-io)

add_executable(dfm-mkdir dfm-mkdir.cpp)
target_link_libraries(dfm-mkdir dfm${DFM_VERSION_MAJOR}-io)

add_executable(dfm-link dfm-link.cpp)
target_link_libraries(dfm-link dfm${DFM_VERSION_MAJOR}-io)

add_executable(dfm-watcher dfm-watcher.cpp)
target_link_libraries(dfm-watcher dfm${DFM_VERSION_MAJOR}-io)

