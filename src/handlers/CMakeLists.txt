
include_directories (${CMAKE_SOURCE_DIR}/src/) 

add_library(onion_handlers STATIC onion_handler_static.c onion_handler_path.c onion_handler_directory.c
																	onion_handler_regexp.c onion_handler_auth_pam.c onion_handler_opack.c)

target_link_libraries(onion_handlers pam pam_misc)
