add_executable(01-hash 01-hash.c)
target_link_libraries(01-hash onion)

add_executable(02-request 02-request.c)
target_link_libraries(02-request onion)

add_executable(03-response 03-response.c)
target_link_libraries(03-response onion)

add_executable(04-handler 04-handler.c)
target_link_libraries(04-handler onion_handlers onion)

add_executable(05-server 05-server.c)
target_link_libraries(05-server onion_handlers onion)

add_executable(06-onion 06-onion.c)
target_link_libraries(06-onion onion_handlers onion)

add_executable(07-codecs 07-codecs.c)
target_link_libraries(07-codecs onion_handlers onion)
