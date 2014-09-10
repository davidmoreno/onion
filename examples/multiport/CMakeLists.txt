include_directories (${PROJECT_SOURCE_DIR}/src/) 

add_executable(multiport multiport.c)

target_link_libraries(multiport onion_static)

