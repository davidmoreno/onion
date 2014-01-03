include_directories (${PROJECT_SOURCE_DIR}/src) 

add_executable(opack opack.c ../common/updateassets.c ../../src/onion/log.c ../../src/onion/mime.c ../../src/onion/dict.c ../../src/onion/block.c ../../src/onion/codecs.c)
target_link_libraries(opack ${PTHREADS_LIB} ${GNUTLS_LIB})

install(TARGETS opack DESTINATION bin)



