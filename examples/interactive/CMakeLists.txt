include_directories (${PROJECT_SOURCE_DIR}/src/) 

add_executable(interactive interactive.c)

target_link_libraries(interactive onion_static)

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/interactive
	DESTINATION ${CMAKE_INSTALL_BINDIR} RENAME onion-interactive
)

