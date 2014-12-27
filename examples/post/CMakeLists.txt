include_directories (${PROJECT_SOURCE_DIR}/src/) 

add_executable(post post.c)
target_link_libraries(post onion)
