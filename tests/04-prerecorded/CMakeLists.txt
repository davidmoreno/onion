
include_directories (${CMAKE_SOURCE_DIR}/src/) 

add_executable(01-prerecorded 01-prerecorded.c)
target_link_libraries(01-prerecorded onion_handlers onion_static)

