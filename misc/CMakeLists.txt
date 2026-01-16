set(MODULE_NAME misc)
add_executable(${MODULE_NAME} misc.c)
target_link_libraries(${MODULE_NAME} PRIVATE tools)