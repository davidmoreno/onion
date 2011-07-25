cmake_minimum_required (VERSION 2.8)
cmake_policy(VERSION 2.8)
project(libonion)

# Some tweak parameters
SET(ONION_USE_SSL true)
SET(ONION_USE_PAM true)
SET(ONION_USE_PTHREADS true)
SET(ONION_USE_PNG true)
SET(ONION_USE_XML2 true)
SET(ONION_VERSION 0.4.0)
########

SET(LIBPATH /usr/lib /usr/local/lib)

IF (NOT DEFINED BINDIR)
SET(BINDIR bin)
ENDIF (NOT DEFINED BINDIR)

IF (NOT DEFINED LIBDIR)
SET(LIBDIR lib)
ENDIF (NOT DEFINED LIBDIR)

IF (NOT DEFINED INCLUDE_PATH)
SET(INCLUDE_PATH /usr/include)
ENDIF (NOT DEFINED INCLUDE_PATH)


IF (NOT DEFINED INCLUDEDIR)
SET(INCLUDEDIR include/onion)
ENDIF(NOT DEFINED INCLUDEDIR)

if (${ONION_USE_SSL})
	find_library(GNUTLS_LIB NAMES gnutls PATH ${LIBPATH})
	find_library(GCRYPT_LIB NAMES gcrypt PATH ${LIBPATH})
	if (GNUTLS_LIB AND GCRYPT_LIB)
		set(GNUTLS_ENABLED true)
		message(STATUS "Gnutls found. SSL support is compiled in.")
	else(GNUTLS_LIB AND GCRYPT_LIB)
		message("Gnutls not found. SSL support is not compiled in.")
	endif(GNUTLS_LIB AND GCRYPT_LIB)
endif(${ONION_USE_SSL})

if (${ONION_USE_PTHREADS})
	find_library(PTHREADS_LIB NAMES pthread PATH ${LIBPATH})
	if(PTHREADS_LIB)
		message(STATUS "pthreads found. Threading support is compiled in.")
		set(PTHREADS true)
	else(PTHREADS_LIB)
		message("pthreads not found. Threading is not supported.")
	endif(PTHREADS_LIB)
endif(${ONION_USE_PTHREADS})

if (${ONION_USE_XML2})
	find_library(XML2_LIB NAMES xml2 PATH ${LIBPATH})
	if(XML2_LIB)
		message(STATUS "libxml2 found. WebDAV support is compiled in.")
		set(XML2_ENABLED true)
	else(XML2_LIB)
		message("libxml2 not found. WebDAV is not supported.")
	endif(XML2_LIB)
endif(${ONION_USE_XML2})

if (${ONION_USE_PAM})
	find_library(PAM_LIB NAMES pam pam_misc PATH ${LIBPATH})
	if (PAM_LIB)
		set(PAM_ENABLED true)
		message(STATUS "pam found. PAM support is compiled in in handlers.")
	else (PAM_LIB)
		message("pam not found. No PAM support.")
  endif (PAM_LIB)
endif (${ONION_USE_PAM})

if (${ONION_USE_PNG})
	find_library(PNG_LIB NAMES png PATH ${LIBPATH})
	if (PNG_LIB)
		set(PNG_ENABLED true)
		set(PNG_LIB ${PNG_LIB})
		message(STATUS "libpng found. png support is compiled in at extras.")
		find_library(CAIRO_LIB NAMES cairo PATH ${LIBPATH})
		find_file(CAIRO_HEADER cairo/cairo.h ${INCLUDE_PATH})
		if (CAIRO_HEADER)
			set(CAIRO_ENABLED true)
			set(CAIRO_LIB ${CAIRO_LIB})
			message(STATUS "libcairo found. Adding cairo examples. ${CAIRO_LIB}")
		else (CAIRO_HEADER)
			message(STATUS "libcairo NOT found.")
		endif (CAIRO_HEADER)
	else (PNG_LIB)
		message("libpng not found. No png support.")
  endif (PNG_LIB)
endif (${ONION_USE_PNG})

find_library(CURL_LIB NAMES curl PATH ${LIBPATH})
if(CURL_LIB)
	message(STATUS "curl found. Some extra test are compiled.")
	set(CURL true)
else(CURL_LIB)
	message("curl not found. Some examples wil not be built.")
endif(CURL_LIB)


# defines
if (GNUTLS_ENABLED)
	add_definitions(-DHAVE_GNUTLS)
endif(GNUTLS_ENABLED)
if (PTHREADS)
	add_definitions(-DHAVE_PTHREADS)
endif(PTHREADS)
if (PAM_ENABLED)
	add_definitions(-DHAVE_PAM)
endif(PAM_ENABLED)


##
IF (${CMAKE_BUILD_TYPE} MATCHES "Debug")
	add_definitions(-D__DEBUG__)
	SET(CMAKE_C_FLAGS "-Wall -Werror -O0")
ELSE (${CMAKE_BUILD_TYPE} MATCHES "Debug")
	SET(CMAKE_C_FLAGS "-Wall -Werror -O2")
ENDIF (${CMAKE_BUILD_TYPE} MATCHES "Debug")


project (onion)

INCLUDE(CPackConfig.cmake)
INCLUDE(CPack)

add_subdirectory(src)

if (CROSS_COMPILING)
else (CROSS_COMPILING)
  add_subdirectory(tools)
	SET(OTEMPLATE otemplate)
	SET(OPACK opack)
endif (CROSS_COMPILING)

add_subdirectory(examples)
add_subdirectory(tests)

