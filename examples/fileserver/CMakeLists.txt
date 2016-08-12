include_directories (${PROJECT_SOURCE_DIR}/src/) 

add_executable(fileserver fileserver.c)
target_link_libraries(fileserver onion_static)

set_target_properties(fileserver PROPERTIES OUTPUT_NAME "onion-fileserver")
install(TARGETS fileserver DESTINATION ${CMAKE_INSTALL_BINDIR})
