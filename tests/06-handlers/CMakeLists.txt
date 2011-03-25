
include_directories (${CMAKE_SOURCE_DIR}/src/) 

add_executable(01-url 01-url.c)
target_link_libraries(01-url onion onion_handlers)


