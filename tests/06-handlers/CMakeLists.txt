
include_directories (${CMAKE_SOURCE_DIR}/src/) 

add_executable(02-exportlocal 02-exportlocal.c buffer.c)
target_link_libraries(02-exportlocal onion onion_handlers)

