include_directories (${CMAKE_SOURCE_DIR}/src/) 

add_executable(userver userver.c)
target_link_libraries(userver onion_handlers onion_static)
