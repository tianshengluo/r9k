set(MODULE_NAME ta)
add_executable(${MODULE_NAME} ta.c wordc.c)
target_link_libraries(${MODULE_NAME} PRIVATE tools)