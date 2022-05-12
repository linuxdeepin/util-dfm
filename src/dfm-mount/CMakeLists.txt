cmake_minimum_required(VERSION 3.0)
project(dfm-mount)

# Version
if (NOT PROJECT_VERSION)
    set(PROJECT_VERSION "1.0.0")
endif()

if (NOT PROJECT_VERSION_MAJOR)
    set(PROJECT_VERSION_MAJOR 0)
endif()

# signals and slots keyword is disable, use Q_SIGNALS and Q_SLOTS instead
add_compile_definitions(QT_NO_SIGNALS_SLOTS_KEYWORDS)

# gio signals conflicts with qt signals
add_definitions(-DQT_NO_KEYWORDS)

find_package(PkgConfig REQUIRED)

# Use Qt modules
set(QtFindModules Core Gui Widgets Concurrent DBus)
foreach(QtModule ${QtFindModules})
    find_package(Qt5 COMPONENTS ${QtModule} REQUIRED)

    # can use target_link_libraries(xxx ${QtUseModules})
    list(APPEND QtUseModules "Qt5::${QtModule}")
    message("QtModule found ${QtModule} OK!")
endforeach()

# Use PkgConfig modules
set(PkgConfigFindModules udisks2 glib-2.0 gio-unix-2.0 libsecret-1)
pkg_check_modules(DEPENDS REQUIRED ${PkgConfigFindModules})
foreach(PkgConfigModule ${PkgConfigFindModules})
    pkg_check_modules(PkgUseModuleVal REQUIRED ${PkgConfigModule})
    #can use target_link_libraries(xxx ${PkgUseModule})
    list(APPEND PkgUseModules ${PkgUseModuleVal_LDFLAGS})
    include_directories(${PkgUseModuleVal_INCLUDE_DIRS})
endforeach(PkgConfigModule)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

FILE(GLOB LIB_BASE_HEADER
    ${CMAKE_CURRENT_LIST_DIR}/include/base/*.h
)
FILE(GLOB LIB_HEADER
    ${CMAKE_CURRENT_LIST_DIR}/include/*.h
)
FILE(GLOB LIB_P_HEADER
    ${CMAKE_CURRENT_LIST_DIR}/private/*.h
)
FILE(GLOB LIB_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/private/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/lib/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/lib/*/*.cpp
)

# Build library
add_library(${PROJECT_NAME} SHARED ${LIB_HEADER} ${LIB_BASE_HEADER} ${LIB_P_HEADER} ${LIB_SRCS})

set_target_properties(
    dfm-mount PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_link_libraries(${PROJECT_NAME}
    ${PkgUseModules}
    ${QtUseModules}
)

include(GNUInstallDirs)

# Install lib
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${LIB_HEADER} DESTINATION include/${PROJECT_NAME})
install(FILES ${LIB_BASE_HEADER} DESTINATION include/${PROJECT_NAME}/base)
configure_file("./lib/dfm-mount.pc.in" "./lib/dfm-mount.pc" @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/lib/dfm-mount.pc"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
# Build example
set(EXAMPLE_SRCS
    example/main.cpp)
add_executable(${PROJECT_NAME}_example ${EXAMPLE_SRCS})
target_link_libraries(${PROJECT_NAME}_example ${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME}_example Qt5::Widgets)

#set(CMAKE_CXX_FLAGS "-fsanitize=undefined,address,leak -fno-omit-frame-pointer")
#set(CMAKE_C_FLAGS "-fsanitize=undefined,address,leak -fno-omit-frame-pointer")
#set(CMAKE_Ｌ_FLAGS "-fsanitize=undefined,address,leak -fno-omit-frame-pointer")

#set(CMAKE_CXX_FLAGS "-fsanitize=thread")
#set(CMAKE_C_FLAGS "-fsanitize=thread")
#set(CMAKE_Ｌ_FLAGS "-fsanitize=thread")
