include_directories (${CMAKE_SOURCE_DIR}/src/) 

add_executable(fileserver fileserver.c)
target_link_libraries(fileserver onion_handlers onion_static)

