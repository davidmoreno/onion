include_directories (${CMAKE_SOURCE_DIR}/src) 

add_executable(opack opack.c ../common/updateassets.c ../../src/onion/log.c)

install(TARGETS opack DESTINATION bin)



