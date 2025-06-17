#add_subdirectory(${PROJECT_SOURCE_DIR}/src/dfm-burn)

# signals and slots keyword is disable, use Q_SIGNALS and Q_SLOTS instead
add_compile_definitions(QT_NO_SIGNALS_SLOTS_KEYWORDS)

add_subdirectory("dfm-io")
add_subdirectory("dfm-mount")
add_subdirectory("dfm-burn")

#add_subdirectory(${PROJECT_SOURCE_DIR}/tests)
