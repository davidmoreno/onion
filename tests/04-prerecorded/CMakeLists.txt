
include_directories (${PROJECT_SOURCE_DIR}/src/) 

add_executable(prerecorded prerecorded.c ../01-internal/buffer_listen_point.c)
target_link_libraries(prerecorded onion_static)

