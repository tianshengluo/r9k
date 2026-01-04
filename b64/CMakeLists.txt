set(MODULE_NAME b64)
add_executable(${MODULE_NAME} b64.c base64.c)
target_link_libraries(${MODULE_NAME} PRIVATE tools)