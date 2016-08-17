# - Find libevent
# Find the native libevent includes and library.
# Once done this will define
#
#  LIBEVENT_INCLUDE_DIRS - where to find event2/event.h, etc.
#  LIBEVENT_LIBRARIES    - List of libraries when using libevent.
#  LIBEVENT_FOUND        - True if libevent found.
#
#  LIBEVENT_VERSION_STRING - The version of libevent found (x.y.z)
#  LIBEVENT_VERSION_MAJOR  - The major version
#  LIBEVENT_VERSION_MINOR  - The minor version
#  LIBEVENT_VERSION_PATCH  - The patch version
#
# Avaliable components:
#  core
#  pthreads
#  extra
#  openssl
#

SET(LIBEVENT_core_LIBRARY "NOTREQ")
SET(LIBEVENT_pthreads_LIBRARY "NOTREQ")
SET(LIBEVENT_extra_LIBRARY "NOTREQ")
SET(LIBEVENT_openssl_LIBRARY "NOTREQ")

FIND_PATH(LIBEVENT_INCLUDE_DIR NAMES event2/event.h)

IF(LIBEVENT_INCLUDE_DIR AND EXISTS "${LIBEVENT_INCLUDE_DIR}/event2/event-config.h")
    # Read and parse libevent version header file for version number
    file(READ "${LIBEVENT_INCLUDE_DIR}/event2/event-config.h" _libevent_HEADER_CONTENTS)

    string(REGEX REPLACE ".*#define _EVENT_VERSION +\"([0-9]+).*" "\\1" LIBEVENT_VERSION_MAJOR "${_libevent_HEADER_CONTENTS}")
    string(REGEX REPLACE ".*#define _EVENT_VERSION +\"[0-9]+\\.([0-9]+).*" "\\1" LIBEVENT_VERSION_MINOR "${_libevent_HEADER_CONTENTS}")
    string(REGEX REPLACE ".*#define _EVENT_VERSION +\"[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" LIBEVENT_VERSION_PATCH "${_libevent_HEADER_CONTENTS}")

    SET(LIBEVENT_VERSION_STRING "${LIBEVENT_VERSION_MAJOR}.${LIBEVENT_VERSION_MINOR}.${LIBEVENT_VERSION_PATCH}")
ENDIF()

FOREACH(component ${Libevent_FIND_COMPONENTS})
    UNSET(LIBEVENT_${component}_LIBRARY)
    FIND_LIBRARY(LIBEVENT_${component}_LIBRARY NAMES event_${component} PATH_SUFFIXES event2)
ENDFOREACH()

# handle the QUIETLY and REQUIRED arguments and set LIBEVENT_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Libevent
        REQUIRED_VARS LIBEVENT_core_LIBRARY LIBEVENT_pthreads_LIBRARY LIBEVENT_extra_LIBRARY LIBEVENT_openssl_LIBRARY LIBEVENT_INCLUDE_DIR
        VERSION_VAR LIBEVENT_VERSION_STRING
        )

IF(LIBEVENT_FOUND)
    SET(LIBEVENT_INCLUDE_DIRS ${LIBEVENT_INCLUDE_DIR})
    FOREACH(component ${Libevent_FIND_COMPONENTS})
        LIST(APPEND LIBEVENT_LIBRARIES ${LIBEVENT_${component}_LIBRARY})
    ENDFOREACH()
ENDIF()

MARK_AS_ADVANCED(LIBEVENT_core_LIBRARY LIBEVENT_pthreads_LIBRARY LIBEVENT_extra_LIBRARY LIBEVENT_openssl_LIBRARY LIBEVENT_INCLUDE_DIR)
