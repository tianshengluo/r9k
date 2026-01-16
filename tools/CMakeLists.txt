set(MODULE_NAME tools)
add_library(${MODULE_NAME} STATIC argparse.c io_utils.c string.c readline.c list.c md5.c)
target_include_directories(${MODULE_NAME} PUBLIC include)