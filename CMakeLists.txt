cmake_minimum_required (VERSION 2.8)
cmake_policy(VERSION 2.8)
project(libonion)

# Some tweak parameters
SET(ONION_USE_SSL true)
SET(ONION_USE_PAM true)
SET(ONION_USE_PTHREADS true)
SET(ONION_USE_PNG true)
SET(ONION_VERSION 0.3.0)
########

SET(LIBPATH /usr/lib /usr/local/lib)
SET(LIBDIR lib)
SET(INCLUDEDIR include/onion)

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
	find_library(PNG_LIB NAMES png z PATH ${LIBPATH})
	if (PNG_LIB)
		set(PNG_ENABLED true)
		set(PNG_LIB ${PNG_LIB} z)
		message(STATUS "libpng found. png support is compiled in at extras.")
	else (PNG_LIB)
		message("libpng not found. No png support.")
  endif (PNG_LIB)
endif (${ONION_USE_PNG})

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

