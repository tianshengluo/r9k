set(MODULE_NAME tools)
add_definitions(-D_XOPEN_SOURCE=700 -D_POSIX_C_SOURCE=200809L)
add_library(${MODULE_NAME} STATIC argparse.c io_utils.c string.c readline.c md5.c)
target_include_directories(${MODULE_NAME} PUBLIC include)