cmake_minimum_required(VERSION 3.26)
project(r9k C)

set(CMAKE_C_STANDARD 17)

include_directories(SYSTEM include)

add_subdirectory(tools)
add_subdirectory(strc)
add_subdirectory(url)
add_subdirectory(clip)
add_subdirectory(b64)
add_subdirectory(calc)
add_subdirectory(rsh)
add_subdirectory(streq)
add_subdirectory(misc)
add_subdirectory(gateway)