include_directories (${PROJECT_SOURCE_DIR}/src/) 

add_executable(interactive interactive.c)

target_link_libraries(interactive onion_static)

set_target_properties(interactive PROPERTIES OUTPUT_NAME "onion-interactive")
install(TARGETS interactive	DESTINATION ${CMAKE_INSTALL_BINDIR})

