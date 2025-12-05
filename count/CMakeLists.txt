set(MODULE_NAME count)
add_executable(${MODULE_NAME} count.c wordc.c)
target_link_libraries(${MODULE_NAME} PRIVATE tools)