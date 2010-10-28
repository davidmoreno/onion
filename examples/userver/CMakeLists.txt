include_directories (${CMAKE_SOURCE_DIR}/src/onion/ ${CMAKE_SOURCE_DIR}/src/handlers/ ${CMAKE_SOURCE_DIR}/src/onionssl/) 

add_executable(userver userver.c)
target_link_libraries(userver onion_handlers onion)

add_executable(userver_ssl userver_ssl.c)
target_link_libraries(userver_ssl onion_handlers onion_ssl onion )

