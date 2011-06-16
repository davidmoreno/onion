# the lib itself. Two versions, static and shared.

add_library(onion SHARED codecs.c dict.c onion.c request.c request_parser.c response.c handler.c 
												server.c log.c sessions.c shortcuts.c block.c mime.c url.c)

add_library(onion_static STATIC codecs.c dict.c onion.c request.c request_parser.c response.c handler.c 
												server.c log.c sessions.c shortcuts.c block.c mime.c url.c)


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

# Only if png by the moment, maybe later other functionalities there.
if (${PNG_ENABLED})
add_subdirectory(extras)
endif (${PNG_ENABLED})

SET(INCLUDES_ONION onion.h dict.h request.h response.h codecs.h handler.h server.h log.h sessions.h types.h shortcuts.h url.h block.h)
MESSAGE("Found include files ${INCLUDES_ONION}")

install(FILES ${INCLUDES_ONION} DESTINATION ${INCLUDEDIR})
install(TARGETS onion onion_static EXPORT onion DESTINATION ${LIBDIR})

if (${ARM})
elseif (${ARM})
install(EXPORT onion DESTINATION share/cmake)
endif (${ARM})

