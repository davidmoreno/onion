include_directories (${CMAKE_SOURCE_DIR}/src/) 

add_executable(interactive interactive.c)

target_link_libraries(interactive onion_static)


