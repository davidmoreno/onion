cmake_minimum_required (VERSION 2.6)

project (onion)

SET(CMAKE_C_FLAGS "-Wall -Werror")

add_subdirectory(src)
add_subdirectory(tests)

add_subdirectory(examples)