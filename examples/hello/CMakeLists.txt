include_directories (${PROJECT_SOURCE_DIR}/src/) 

add_executable(hello hello.c)
target_link_libraries(hello onion)




