
include_directories (${CMAKE_SOURCE_DIR}/src) 

add_library(onion_handlers STATIC onion_handler_static.c onion_handler_path.c onion_handler_directory.c)

