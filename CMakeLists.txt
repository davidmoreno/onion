cmake_minimum_required (VERSION 2.8)
cmake_policy(VERSION 2.8)
project(libonion)

# Some tweak parameters
SET(ONION_USE_SSL true)
SET(ONION_USE_PAM true)
SET(ONION_USE_PTHREADS true)
SET(ONION_VERSION 0.2.0)
########

if (${ARM} MATCHES "yes")
	message(STATUS "Compiling for ARM")
	SET(ONION_USE_SSL false)
	SET(ONION_USE_PAM false)
	SET(CMAKE_C_COMPILER /usr/bin/arm-linux-gnueabi-gcc)
endif (${ARM} MATCHES "yes")


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


if (${ONION_USE_PAM})
	find_library(PAM_LIB NAMES pam pam_misc PATH /usr/lib /usr/local/lib)
	if (PAM_LIB)
		set(PAM_ENABLED true)
		message(STATUS "pam found. PAM support is compiled in in handlers.")
	else (PAM_LIB)
		message(STATUS "pam not found. No PAM support.")
  endif (PAM_LIB)
endif (${ONION_USE_PAM})

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
add_subdirectory(tools)

add_subdirectory(examples)
add_subdirectory(tests)
