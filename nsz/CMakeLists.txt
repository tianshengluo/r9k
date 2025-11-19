set(MODULE_NAME nsz)
add_executable(${MODULE_NAME} nsz.c)
target_link_libraries(${MODULE_NAME} PRIVATE tools)