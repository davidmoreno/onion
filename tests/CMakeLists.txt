include_directories (${CMAKE_SOURCE_DIR}/src/onion/ ${CMAKE_SOURCE_DIR}/src/handlers/) 

add_subdirectory(01-internal)

add_subdirectory(02-fullserver)
