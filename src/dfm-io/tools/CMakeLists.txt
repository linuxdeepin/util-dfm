project(tools)

add_definitions(-DQT_NO_KEYWORDS)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -fsanitize=address")

# include
include_directories(
    ${PROJECT_SOURCE_DIR}/../dfm-io/inlcude
    ${PROJECT_SOURCE_DIR}/../dfm-io/private
)

add_executable(dfm-list dfm-list.cpp)
target_link_libraries(dfm-list dfm-io)

add_executable(dfm-info dfm-info.cpp)
target_link_libraries(dfm-info dfm-io)

add_executable(dfm-copy dfm-copy.cpp)
target_link_libraries(dfm-copy dfm-io)

add_executable(dfm-copy-gio dfm-copy-gio.cpp)
target_link_libraries(dfm-copy-gio dfm-io)

#add_executable(dfm-copy3 dfm-copy3.cpp)
#target_link_libraries(dfm-copy3 dfm-io)

add_executable(dfm-copy-system dfm-copy-system.cpp)
target_link_libraries(dfm-copy-system dfm-io)

add_executable(dfm-cat dfm-cat.cpp)
target_link_libraries(dfm-cat dfm-io)

add_executable(dfm-save dfm-save.cpp)
target_link_libraries(dfm-save dfm-io)

add_executable(dfm-rename dfm-rename.cpp)
target_link_libraries(dfm-rename dfm-io)

add_executable(dfm-move dfm-move.cpp)
target_link_libraries(dfm-move dfm-io)

add_executable(dfm-trash dfm-trash.cpp)
target_link_libraries(dfm-trash dfm-io)

add_executable(dfm-delete dfm-delete.cpp)
target_link_libraries(dfm-delete dfm-io)

add_executable(dfm-restore dfm-restore.cpp)
target_link_libraries(dfm-restore dfm-io)

add_executable(dfm-touch dfm-touch.cpp)
target_link_libraries(dfm-touch dfm-io)

add_executable(dfm-mkdir dfm-mkdir.cpp)
target_link_libraries(dfm-mkdir dfm-io)

add_executable(dfm-link dfm-link.cpp)
target_link_libraries(dfm-link dfm-io)

add_executable(dfm-watcher dfm-watcher.cpp)
target_link_libraries(dfm-watcher dfm-io)

#install (
#    FILES
#        ${CMAKE_CURRENT_BINARY_DIR}/dfm-list
#        ${CMAKE_CURRENT_BINARY_DIR}/dfm-info
#        ${CMAKE_CURRENT_BINARY_DIR}/dfm-copy
#        ${CMAKE_CURRENT_BINARY_DIR}/dfm-cat
#        ${CMAKE_CURRENT_BINARY_DIR}/dfm-save
#        ${CMAKE_CURRENT_BINARY_DIR}/dfm-rename
#        ${CMAKE_CURRENT_BINARY_DIR}/dfm-move
#        ${CMAKE_CURRENT_BINARY_DIR}/dfm-trash
#        ${CMAKE_CURRENT_BINARY_DIR}/dfm-delete
#        ${CMAKE_CURRENT_BINARY_DIR}/dfm-restore
#        ${CMAKE_CURRENT_BINARY_DIR}/dfm-touch
#        ${CMAKE_CURRENT_BINARY_DIR}/dfm-mkdir
#        ${CMAKE_CURRENT_BINARY_DIR}/dfm-link
#        ${CMAKE_CURRENT_BINARY_DIR}/dfm-watcher
#    DESTINATION /usr/bin/
#    PERMISSIONS WORLD_EXECUTE
#)

install (
    TARGETS
        dfm-list
        dfm-info
        dfm-copy
        dfm-cat
        dfm-save
        dfm-rename
        dfm-move
        dfm-trash
        dfm-delete
        dfm-restore
        dfm-touch
        dfm-mkdir
        dfm-link
        dfm-watcher
    DESTINATION /usr/bin/
    PERMISSIONS WORLD_EXECUTE
)
