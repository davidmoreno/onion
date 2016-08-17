# - Try to find LibEv
# Once done this will define
#
#  LIBEV_FOUND - system has libev
#  LIBEV_INCLUDE_DIR - the libev include directory
#  LIBEV_LIBRARIES - Link these to use libev
#

set(_LIBEV_ROOT_HINTS
    $ENV{GCRYTPT_ROOT_DIR}
    ${LIBEV_ROOT_DIR})

set(_LIBEV_ROOT_PATHS
    "$ENV{PROGRAMFILES}/libev")

set(_LIBEV_ROOT_HINTS_AND_PATHS
    HINTS ${_LIBEV_ROOT_HINTS}
    PATHS ${_LIBEV_ROOT_PATHS})


find_path(LIBEV_INCLUDE_DIR
    NAMES
        ev.h
    HINTS
        ${_LIBEV_ROOT_HINTS_AND_PATHS}
)

find_library(LIBEV_LIBRARY
    NAMES
        ev
    HINTS
        ${_LIBEV_ROOT_HINTS_AND_PATHS}
)
set(LIBEV_LIBRARIES ${LIBEV_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibEv
    REQUIRED_VARS
        LIBEV_INCLUDE_DIR
        LIBEV_LIBRARIES
    FAIL_MESSAGE
        "Could NOT find LibEv, try to set the path to LibEv root folder in the system variable LIBEV_ROOT_DIR"
)

# show the LIBEV_INCLUDE_DIRS and LIBEV_LIBRARIES variables only in the advanced view
mark_as_advanced(LIBEV_INCLUDE_DIR LIBEV_LIBRARIES)
