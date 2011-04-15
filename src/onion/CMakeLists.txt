# the lib itself. Two versions, static and shared.
add_library(onion SHARED codecs.c dict.c onion.c request.c request_parser.c response.c handler.c 
												server.c log.c sessions.c shortcuts.c block.c)

add_library(onion_static STATIC codecs.c dict.c onion.c request.c request_parser.c response.c handler.c 
												server.c log.c sessions.c shortcuts.c block.c)

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

SET(INCLUDES_ONION onion.h dict.h request.h response.h codecs.h handler.h server.h log.h sessions.h types.h shortcuts.h)
MESSAGE("Found include files ${INCLUDES_ONION}")

install(FILES ${INCLUDES_ONION} DESTINATION ${INCLUDEDIR})
install(TARGETS onion onion_static EXPORT onion DESTINATION ${LIBDIR})
install(EXPORT onion DESTINATION share/cmake)

