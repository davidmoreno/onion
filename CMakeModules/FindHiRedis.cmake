# - Find Redis client db (hiredis from https://github.com/antirez/hiredis)
# Find the hiredis includes and client library
# This module defines
#  HIREDIS_INCLUDE_DIR, where to find hiredis/hiredis.h
#  HIREDIS_LIBRARIES, the libraries needed to use redis.
#  HIREDIS_FOUND, If false, do not try to use redis.
#
# Copyright (c) 2012, Lothar Braun, <braun@net.in.tum.de>
#

# Add the redis include paths here
IF (HIREDIS_INCLUDE_DIR AND HIREDIS_LIBRARIES)
	SET (HIREDIS_FOUND TRUE)
ELSE(HIREDIS_INCLUDE_DIR AND HIREDIS_LIBRARIES)
	FIND_PATH(HIREDIS_INCLUDE_DIR hiredis/hiredis.h
		/usr/include/
		/usr/include/hiredis
		/usr/local/include/
		/usr/local/include/hiredis
		/opt/local/include/
		/opt/local/include/hiredis
	)
	
	FIND_LIBRARY(HIREDIS_LIBRARIES NAMES hiredis  libhiredis 
		PATHS
			/usr/lib
			/usr/local/lib
			/opt/local/lib
	)

	IF(HIREDIS_INCLUDE_DIR AND HIREDIS_LIBRARIES)
		SET(HIREDIS_FOUND TRUE)
		MESSAGE(STATUS "Found hiredis: ${HIREDIS_INCLUDE_DIR}, ${HIREDIS_LIBRARIES}")
		INCLUDE_DIRECTORIES(${HIREDIS_INCLUDE_DIR})
	ELSE(HIREDIS_INCLUDE_DIR AND HIREDIS_LIBRARIES)
		SET(HIREDIS_FOUND FALSE)
		MESSAGE(STATUS "hiredis client library not found.")
	ENDIF(HIREDIS_INCLUDE_DIR AND HIREDIS_LIBRARIES)
  
	MARK_AS_ADVANCED(HIREDIS_INCLUDE_DIR HIREDIS_LIBRARIES)
ENDIF(HIREDIS_INCLUDE_DIR AND HIREDIS_LIBRARIES)
