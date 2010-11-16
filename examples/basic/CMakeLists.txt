include_directories (${CMAKE_SOURCE_DIR}/src/) 

add_executable(basic basic.c)
target_link_libraries(basic onion_handlers onion  )


