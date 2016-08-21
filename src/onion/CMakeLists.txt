# the lib itself. Two versions, static and shared.
add_subdirectory(handlers)
include_directories (${PROJECT_SOURCE_DIR}/src/)


SET(INCLUDES block.h codecs.h dict.h handler.h http.h https.h listen_point.h low.h log.h mime.h onion.h poller.h
	request.h response.h sessions.h shortcuts.h types.h types_internal.h url.h websocket.h ptr_list.h)

set(SOURCES onion.c codecs.c dict.c low.c request.c response.c handler.c log.c sessions.c sessions_mem.c shortcuts.c
	block.c mime.c url.c listen_point.c request_parser.c http.c websocket.c ptr_list.c
	handlers/static.c handlers/exportlocal.c handlers/opack.c handlers/path.c handlers/internal_status.c
	version.c
	)

set(LIBRARIES)
LIST(APPEND LIBRARIES rt)

## optional add sources and libraries depeding on features

# Poller can be a bit more complex, but basically the same.
if (${ONION_POLLER} STREQUAL libevent)
	find_package(Libevent COMPONENTS core pthreads REQUIRED)
	LIST(APPEND SOURCES poller_libevent.c)
	include_directories(${LIBEVENT_INCLUDE_DIRS})
endif (${ONION_POLLER} STREQUAL libevent)
if (${ONION_POLLER} STREQUAL libev)
	find_package(LibEv REQUIRED)
	LIST(APPEND SOURCES poller_libev.c)
	include_directories(${LIBEV_INCLUDE_DIR})
endif (${ONION_POLLER} STREQUAL libev)
if (${ONION_POLLER} STREQUAL epoll)
	LIST(APPEND SOURCES poller.c)
endif (${ONION_POLLER} STREQUAL epoll)

# library dependencies
if (${SQLITE3_ENABLED})
	LIST(APPEND SOURCES sessions_sqlite3.c)
	LIST(APPEND INCLUDES sessions_sqlite3.h)
endif(${SQLITE3_ENABLED})
if (${REDIS_ENABLED})
        LIST(APPEND SOURCES sessions_redis.c)
        LIST(APPEND INCLUDES sessions_redis.h)
endif(${REDIS_ENABLED})
if (${GNUTLS_ENABLED})
	MESSAGE(STATUS "Compiling HTTPS support")
	LIST(APPEND SOURCES https.c random-gnutls.c)
else(${GNUTLS_ENABLED})
	LIST(APPEND SOURCES random-default.c)
endif(${GNUTLS_ENABLED})
if (${XML2_ENABLED})
	MESSAGE(STATUS "Compiling WebDAV support")
	include_directories(${LIBXML2_INCLUDE_DIR})
	LIST(APPEND SOURCES handlers/webdav.c)
	LIST(APPEND LIBRARIES ${LIBXML2_LIBRARIES})
endif (${XML2_ENABLED})
if (${PAM_ENABLED})
	LIST(APPEND SOURCES handlers/auth_pam.c)
	LIST(APPEND LIBRARIES ${PAM_LIBRARY})
endif (${PAM_ENABLED})
if (GNUTLS_ENABLED)
	LIST(APPEND LIBRARIES ${GCRYPT_LIBRARIES} ${GNUTLS_LIBRARIES})
endif(GNUTLS_ENABLED)
if (PTHREADS)
	LIST(APPEND LIBRARIES pthread)
endif(PTHREADS)
if (SQLITE3_ENABLED)
	LIST(APPEND LIBRARIES ${SQLITE3_LIBRARIES})
endif(SQLITE3_ENABLED)
if (REDIS_ENABLED)
	LIST(APPEND LIBRARIES ${HIREDIS_LIBRARIES})
endif(REDIS_ENABLED)
if (${ONION_POLLER} STREQUAL libevent)
	LIST(APPEND LIBRARIES ${LIBEVENT_LIBRARIES})
endif (${ONION_POLLER} STREQUAL libevent)
if (${ONION_POLLER} STREQUAL libev)
	LIST(APPEND LIBRARIES ${LIBEV_LIBRARIES})
endif (${ONION_POLLER} STREQUAL libev)
if (SYSTEMD_ENABLED)
	LIST(APPEND LIBRARIES ${SYSTEMD_LIBRARIES})
endif(SYSTEMD_ENABLED)

IF (${CMAKE_BUILD_TYPE} MATCHES "Fast")
 add_custom_command(
   OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/all-onion.c
   COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR} && cat ${SOURCES}> ${CMAKE_CURRENT_BINARY_DIR}/all-onion.c
   DEPENDS ${SOURCES}
   )
 include_directories(${CMAKE_CURRENT_SOURCE_DIR})
 include_directories(${CMAKE_CURRENT_SOURCE_DIR}/handlers/)
 add_library(onion SHARED ${CMAKE_CURRENT_BINARY_DIR}/all-onion.c)
 add_library(onion_static STATIC ${CMAKE_CURRENT_BINARY_DIR}/all-onion.c)
ELSE (${CMAKE_BUILD_TYPE} MATCHES "Fast")
 add_library(onion SHARED ${SOURCES})
 add_library(onion_static STATIC  ${SOURCES})
ENDIF (${CMAKE_BUILD_TYPE} MATCHES "Fast")

target_link_libraries(onion ${LIBRARIES})
target_link_libraries(onion_static ${LIBRARIES})

# Only if png by the moment, maybe later other functionalities there.
if (PNG_ENABLED OR JPEG_ENABLED)
add_subdirectory(extras)
endif (PNG_ENABLED OR JPEG_ENABLED)

MESSAGE(STATUS "Found include files ${INCLUDES_ONION}")

SET_TARGET_PROPERTIES(onion onion_static
	PROPERTIES
		SOVERSION 0
		VERSION ${ONION_VERSION}
	)

configure_file(version.h.in version.h)

install(FILES ${INCLUDES} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/version.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS onion onion_static EXPORT onion DESTINATION ${CMAKE_INSTALL_LIBDIR})

if (${ARM})
elseif (${ARM})
install(EXPORT onion DESTINATION share/cmake)
endif (${ARM})
