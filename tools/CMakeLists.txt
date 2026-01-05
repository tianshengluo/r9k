set(MODULE_NAME tools)
add_library(${MODULE_NAME} STATIC argparse.c io.c string.c)
target_include_directories(${MODULE_NAME} PUBLIC include)