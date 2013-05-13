include_directories (${PROJECT_SOURCE_DIR}/src) 

add_executable(opack opack.c ../common/updateassets.c ../../src/onion/log.c)

install(TARGETS opack DESTINATION bin)



