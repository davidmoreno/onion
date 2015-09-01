include_directories (${PROJECT_SOURCE_DIR}/src/)
add_executable(longpoll longpoll.c)
target_link_libraries(longpoll onion_static)
