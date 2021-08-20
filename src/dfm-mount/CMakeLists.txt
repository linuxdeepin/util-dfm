cmake_minimum_required(VERSION 3.0)
project(dfm-mount)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 COMPONENTS Core REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(DEPENDS REQUIRED udisks2 gio-2.0)

include_directories(include
    /usr/include/udisks2)

include(GNUInstallDirs)

set(LIB_HEADER 
    include/dfmmount_global.h
    include/dfmmountdefines.h
    include/dfmdeviceinterface.h
    include/dfmmonitorinterface.h
    include/dfmabstractdevice.h
    include/dfmblockdevice.h
    include/dfmabstractmonitor.h
    include/dfmblockmonitor.h)

set(LIB_P_HEADER
    lib/private/dfmblockdevice_p.h
    lib/private/dfmblockmonitor_p.h)

set(LIB_SRCS
    lib/dfmabstractdevice.cpp
    lib/dfmblockdevice.cpp
    lib/dfmabstractmonitor.cpp
    lib/dfmblockmonitor.cpp)

set(INSTALL_HEADERS
    dfmmount_global.h)


add_library(${PROJECT_NAME} SHARED ${LIB_HEADER} ${LIB_P_HEADER} ${LIB_SRCS})
target_link_libraries(${PROJECT_NAME}
    Qt5::Core
    ${DEPENDS_LIBRARIES})

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${INSTALL_HEADERS} DESTINATION include/${PROJECT_NAME})
configure_file("./lib/dfm-mount.pc.in" "./lib/dfm-mount.pc" @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dfm-mount.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

set(EXAMPLE_SRCS
    example/main.cpp)
add_executable(${PROJECT_NAME}_example ${EXAMPLE_SRCS})
target_link_libraries(${PROJECT_NAME}_example ${PROJECT_NAME})
