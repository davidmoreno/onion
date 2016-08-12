include_directories (${PROJECT_SOURCE_DIR}/src/) 

add_executable(jsonrpc jsonrpc.c)
target_link_libraries(jsonrpc onion)






