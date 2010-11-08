if (${ONION_USE_SSL})
	find_library(GNUTLS_LIB NAMES gnutls PATH /usr/lib /usr/local/lib )
	find_library(GCRYPT_LIB NAMES gcrypt PATH /usr/lib /usr/local/lib )
	if (GNUTLS_LIB AND GCRYPT_LIB)
		set(GNUTLS_ENABLED true)
		message(STATUS "Gnutls found. SSL support is compiled in.")
	else(GNUTLS_LIB AND GCRYPT_LIB)
		message(WARNING "Gnutls not found. SSL support is not compiled in.")
	endif(GNUTLS_LIB AND GCRYPT_LIB)
endif(${ONION_USE_SSL})

if (${ONION_USE_PTHREADS})
	find_library(PTHREADS_LIB NAMES pthread PATH /usr/lib /usr/local/lib )
	if(PTHREADS_LIB)
		message(STATUS "pthreads found. Threading support is compiled in.")
		set(PTHREADS true)
	else(PTHREADS_LIB)
		message(WARNING "pthreads not found. Threading is not supported.")
	endif(PTHREADS_LIB)
endif(${ONION_USE_PTHREADS})

# defines
if (GNUTLS_ENABLED)
	add_definitions(-DHAVE_GNUTLS)
endif(GNUTLS_ENABLED)
if (PTHREADS)
	add_definitions(-DHAVE_PTHREADS)
endif(PTHREADS)

# the lib itself. Two versions, static and shared.
add_library(onion SHARED onion_codecs.c onion_dict.c onion.c onion_request.c onion_response.c onion_handler.c onion_server.c)

add_library(onion_static STATIC onion_codecs.c onion_dict.c onion.c onion_request.c onion_response.c onion_handler.c onion_server.c)

# library dependencies
if (GNUTLS_ENABLED)
	target_link_libraries(onion gnutls gcrypt)
	target_link_libraries(onion_static gnutls gcrypt)
endif(GNUTLS_ENABLED)
if (PTHREADS)
	target_link_libraries(onion pthread)
	target_link_libraries(onion_static pthread)
endif(PTHREADS)
