set(MODULE_NAME test_readline)
add_executable(${MODULE_NAME} readline.c)
target_link_libraries(${MODULE_NAME} PRIVATE tools)