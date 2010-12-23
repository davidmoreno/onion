# the lib itself. Two versions, static and shared.
add_library(onion SHARED codecs.c dict.c onion.c request.c request_parser.c response.c handler.c server.c log.c sessions.c)

add_library(onion_static STATIC codecs.c dict.c onion.c request.c request_parser.c response.c handler.c server.c log.c sessions.c)

# library dependencies
if (GNUTLS_ENABLED)
	target_link_libraries(onion gnutls gcrypt)
	target_link_libraries(onion_static gnutls gcrypt)
endif(GNUTLS_ENABLED)
if (PTHREADS)
	target_link_libraries(onion pthread)
	target_link_libraries(onion_static pthread)
endif(PTHREADS)

add_subdirectory(handlers)
