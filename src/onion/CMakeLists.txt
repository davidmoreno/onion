# the lib itself. Two versions, static and shared.
include_directories (${PROJECT_SOURCE_DIR}/src/) 

if (SQLITE3_ENABLED)
	set(SQLITE3_C sessions_sqlite3.c)
	set(SQLITE3_H sessions_sqlite3.h)
endif(SQLITE3_ENABLED)

# library dependencies
if (GNUTLS_ENABLED)
	set(HTTPS_C https.c)
	set(RANDOM_C random-gnutls.c)
else(GNUTLS_ENABLED)
	set(RANDOM_C random-default.c)
endif(GNUTLS_ENABLED)

if (${ONION_POLLER} STREQUAL libevent)
	set(POLLER_C poller_libevent.c)
endif (${ONION_POLLER} STREQUAL libevent)

if (${ONION_POLLER} STREQUAL libev)
	set(POLLER_C poller_libev.c)
	find_path(LIBEV_HEADER ev.h ${CMAKE_INCLUDE_PATH} /usr/local/include/)
	find_library(LIBEV_LIB NAMES ev PATH /usr/local/lib/)
	if (LIBEV_LIB)
		message(STATUS "libev found at ${LIBEV_LIB}")
	else (LIBEV_LIB)
		message(FATAL_ERROR "libev not found. Cant compile. ${LIBEV_HEADER} ${LIBEV_LIB}")
	endif (LIBEV_LIB)
	include_directories(${LIBEV_HEADER})
endif (${ONION_POLLER} STREQUAL libev)

if (${ONION_POLLER} STREQUAL epoll)
	set(POLLER_C poller.c)
endif (${ONION_POLLER} STREQUAL epoll)


if (PAM_ENABLED)
	SET(PAM handlers/auth_pam.c)
	SET(PAMLIBS pam pam_misc)
else (PAM_ENABLED)
	SET(PAM)
	SET(PAMLIBS)
endif (PAM_ENABLED)

if (${XML2_ENABLED})
	MESSAGE(STATUS "Compiling WebDAV support") 
	SET(WEBDAV handlers/webdav.c)
	SET(XMLLIBS ${XML2_LIB})
	include_directories(${XML2_HEADERS})
endif (${XML2_ENABLED})


SET(SOURCES_HANDLERS handlers/static.c handlers/exportlocal.c handlers/opack.c handlers/path.c handlers/internal_status.c
		${PAM} ${WEBDAV})
SET(LIBRARY_HANDLERS ${PAMLIBS} ${XMLLIBS})

set(SOURCES onion.c codecs.c dict.c low.c request.c response.c handler.c 
                        log.c sessions.c sessions_mem.c shortcuts.c block.c mime.c url.c ${POLLER_C}
			listen_point.c request_parser.c http.c ${HTTPS_C} websocket.c ${RANDOM_C} ${SQLITE3_C} 
			${SOURCES_HANDLERS})

add_subdirectory(handlers)

IF (${CMAKE_BUILD_TYPE} MATCHES "Fast")
 add_custom_command(
   OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/all-onion.c
   COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR} && cat ${SOURCES}> ${CMAKE_CURRENT_BINARY_DIR}/all-onion.c
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
if (SQLITE3_ENABLED)
	target_link_libraries(onion ${SQLITE3_LIB})
	target_link_libraries(onion_static ${SQLITE3_LIB})
endif(SQLITE3_ENABLED)
if (${ONION_POLLER} STREQUAL libevent)
	target_link_libraries(onion event_core event_pthreads)
	target_link_libraries(onion_static event_core event_pthreads)
endif (${ONION_POLLER} STREQUAL libevent)
if (${ONION_POLLER} STREQUAL libev)
	target_link_libraries(onion ${LIBEV_LIB})
	target_link_libraries(onion_static ${LIBEV_LIB})
endif (${ONION_POLLER} STREQUAL libev)
if (SYSTEMD_ENABLED)
	target_link_libraries(onion ${SD_DAEMON_LIB})
	target_link_libraries(onion_static ${SD_DAEMON_LIB})
endif(SYSTEMD_ENABLED)



# Only if png by the moment, maybe later other functionalities there.
if (${PNG_ENABLED})
add_subdirectory(extras)
endif (${PNG_ENABLED})

SET(INCLUDES_ONION block.h codecs.h dict.h handler.h http.h https.h listen_point.h low.h log.h mime.h onion.h poller.h request.h response.h server.h sessions.h shortcuts.h types.h types_internal.h url.h websocket.h ${SQLITE3_H})
MESSAGE(STATUS "Found include files ${INCLUDES_ONION}")

SET_TARGET_PROPERTIES(onion onion_static
	PROPERTIES
		SOVERSION 0
		VERSION ${ONION_VERSION}
	)

install(FILES ${INCLUDES_ONION} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS onion onion_static EXPORT onion DESTINATION ${CMAKE_INSTALL_LIBDIR})

if (${ARM})
elseif (${ARM})
install(EXPORT onion DESTINATION share/cmake)
endif (${ARM})

