include_directories (${CMAKE_SOURCE_DIR}/src/onion/ ${CMAKE_SOURCE_DIR}/src/handlers/ ${CMAKE_SOURCE_DIR}/src/onionssl/) 

add_executable(userver userver.c)
target_link_libraries(userver onion_handlers onion_static)
