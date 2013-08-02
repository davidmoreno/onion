# the lib itself. Two versions, static and shared.

if (SYSTEMD_ENABLED)
	set(SYSTEMD_C sd-daemon.c)
endif(SYSTEMD_ENABLED)
# library dependencies
if (GNUTLS_ENABLED)
	set(HTTPS_C https.c)
endif(GNUTLS_ENABLED)

set(SOURCES onion.c codecs.c dict.c request.c response.c handler.c 
                        log.c sessions.c shortcuts.c block.c mime.c url.c ${SYSTEMD_C} poller.c
                        listen_point.c http_parser.c http.c ${HTTPS_C} websocket.c ro_block.c)

IF (${CMAKE_BUILD_TYPE} MATCHES "Fast")
 add_custom_command(
   OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/all-onion.c
   COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR} && cat ${SOURCES} > ${CMAKE_CURRENT_BINARY_DIR}/all-onion.c
   DEPENDS ${SOURCES}
   )
 include_directories(${CMAKE_CURRENT_SOURCE_DIR})
 add_library(onion SHARED ${CMAKE_CURRENT_BINARY_DIR}/all-onion.c)
 add_library(onion_static STATIC ${CMAKE_CURRENT_BINARY_DIR}/all-onion.c)
ELSE (${CMAKE_BUILD_TYPE} MATCHES "Fast")
 add_library(onion SHARED ${SOURCES})
 add_library(onion_static STATIC  ${SOURCES})
ENDIF (${CMAKE_BUILD_TYPE} MATCHES "Fast")

# library dependencies
if (GNUTLS_ENABLED)
	target_link_libraries(onion ${GNUTLS_LIB} gcrypt)
	target_link_libraries(onion_static ${GNUTLS_LIB} gcrypt)
endif(GNUTLS_ENABLED)
if (PTHREADS)
	target_link_libraries(onion pthread)
	target_link_libraries(onion_static pthread)
endif(PTHREADS)
if (SYSTEMD_ENABLED)
	target_link_libraries(onion ${RT_LIB})
	target_link_libraries(onion_static ${RT_LIB})
endif(SYSTEMD_ENABLED)



add_subdirectory(handlers)

# Only if png by the moment, maybe later other functionalities there.
if (${PNG_ENABLED})
add_subdirectory(extras)
endif (${PNG_ENABLED})

SET(INCLUDES_ONION block.h codecs.h dict.h handler.h http.h https.h listen_point.h log.h mime.h onion.h poller.h request.h response.h sd-daemon.h server.h sessions.h shortcuts.h types.h types_internal.h url.h websocket.h)
MESSAGE("Found include files ${INCLUDES_ONION}")

install(FILES ${INCLUDES_ONION} DESTINATION ${INCLUDEDIR})
install(TARGETS onion onion_static EXPORT onion DESTINATION ${LIBDIR})

if (${ARM})
elseif (${ARM})
install(EXPORT onion DESTINATION share/cmake)
endif (${ARM})

