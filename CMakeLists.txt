cmake_minimum_required (VERSION 2.8)
cmake_policy(VERSION 2.8)

# Some tweak parameters
SET(ONION_USE_SSL true)
SET(ONION_USE_PTHREADS true)

##

project (onion)

SET(CMAKE_C_FLAGS "-Wall -Werror")

add_subdirectory(src)
add_subdirectory(tools)

add_subdirectory(examples)
add_subdirectory(tests)
