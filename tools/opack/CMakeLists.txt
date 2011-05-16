include_directories (${CMAKE_SOURCE_DIR}/src) 

add_executable(opack opack.c)

install(TARGETS opack DESTINATION bin)



