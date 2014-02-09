include_directories (${PROJECT_SOURCE_DIR}/src/) 

add_executable(fileserver fileserver.c)
target_link_libraries(fileserver onion_handlers onion_static)

install(TARGETS fileserver DESTINATION ${CMAKE_INSTALL_BINDIR})
