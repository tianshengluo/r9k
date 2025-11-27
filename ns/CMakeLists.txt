set(MODULE_NAME ns)
add_executable(${MODULE_NAME} ns.c)
target_link_libraries(${MODULE_NAME} PRIVATE tools -lresolv)