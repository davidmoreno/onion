include_directories (${CMAKE_SOURCE_DIR}/src/onion/ ${CMAKE_SOURCE_DIR}/src/handlers/) 

add_executable(userver userver.c)
target_link_libraries(userver onion_handlers onion)

