set(MODULE_NAME copy)
add_executable(${MODULE_NAME} copy.c)
target_link_libraries(${MODULE_NAME} PRIVATE tools)