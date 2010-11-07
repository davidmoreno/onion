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


if (GNUTLS_ENABLED)
	add_definitions(-DHAVE_GNUTLS)
endif(GNUTLS_ENABLED)

add_library(onion onion_codecs.c onion_dict.c onion.c onion_request.c onion_response.c onion_handler.c onion_server.c)

if (GNUTLS_ENABLED)
	target_link_libraries(onion gnutls gcrypt)
endif(GNUTLS_ENABLED)
