include_directories (${PROJECT_SOURCE_DIR}/src) 

add_executable(opack opack.c ../common/updateassets.c ../../src/onion/log.c ../../src/onion/low.c ../../src/onion/mime.c ../../src/onion/dict.c ../../src/onion/block.c ../../src/onion/codecs.c)

if(PTHREADS_LIB)
	target_link_libraries(opack ${PTHREADS_LIB})
endif(PTHREADS_LIB)

if(GNUTLS_ENABLED)
	include_directories(${GNUTLS_INCLUDE_DIR})
	target_link_libraries(opack ${GNUTLS_LIBRARIES})
endif(GNUTLS_ENABLED)

install(TARGETS opack DESTINATION ${CMAKE_INSTALL_BINDIR})



