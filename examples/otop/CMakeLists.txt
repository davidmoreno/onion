include_directories (${CMAKE_SOURCE_DIR}/src) 

add_executable(otop otop.c)
target_link_libraries(otop onion)

