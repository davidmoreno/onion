# the lib itself. Two versions, static and shared.
add_library(onion SHARED onion_codecs.c onion_dict.c onion.c onion_request.c onion_response.c onion_handler.c onion_server.c onion_log.c)

add_library(onion_static STATIC onion_codecs.c onion_dict.c onion.c onion_request.c onion_response.c onion_handler.c onion_server.c onion_log.c)

# library dependencies
if (GNUTLS_ENABLED)
	target_link_libraries(onion gnutls gcrypt)
	target_link_libraries(onion_static gnutls gcrypt)
endif(GNUTLS_ENABLED)
if (PTHREADS)
	target_link_libraries(onion pthread)
	target_link_libraries(onion_static pthread)
endif(PTHREADS)
