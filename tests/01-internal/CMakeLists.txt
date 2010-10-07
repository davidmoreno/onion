add_executable(01-hash 01-hash.c)

target_link_libraries(01-hash onion)


add_executable(02-request 02-request.c)

target_link_libraries(02-request onion)

add_executable(03-response 03-response.c)

target_link_libraries(03-response onion)
