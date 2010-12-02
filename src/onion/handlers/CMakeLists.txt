
include_directories (${CMAKE_SOURCE_DIR}/src/) 

add_library(onion_handlers STATIC static.c path.c directory.c
																	regexp.c auth_pam.c opack.c)

target_link_libraries(onion_handlers pam pam_misc)
