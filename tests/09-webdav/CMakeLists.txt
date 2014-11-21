include_directories (${PROJECT_SOURCE_DIR}/src/) 

add_executable(wdserver wdserver.c)
target_link_libraries(wdserver onion)
 

