set(MODULE_NAME streq)
add_executable(${MODULE_NAME} streq.c)
target_link_libraries(${MODULE_NAME} PRIVATE tools)