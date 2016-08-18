#.rst:
# FindIntl
# --------
#
# Find the Gettext libintl headers and libraries.
#
# This module reports information about the Gettext libintl
# installation in several variables.  General variables::
#
#   INTL_FOUND - true if the libintl headers and libraries were found
#   INTL_INCLUDE_DIRS - the directory containing the libintl headers
#   INTL_LIBRARIES - libintl libraries to be linked
#
# The following cache variables may also be set::
#
#   INTL_INCLUDE_DIR - the directory containing the libintl headers
#   INTL_LIBRARY - the libintl library (if any)
#
# .. note::
#   On some platforms, such as Linux with GNU libc, the gettext
#   functions are present in the C standard library and libintl
#   is not required.  ``INTL_LIBRARIES`` will be empty in this
#   case.
#
# .. note::
#   If you wish to use the Gettext tools (``msgmerge``,
#   ``msgfmt``, etc.), use :module:`FindGettext`.


# Written by Roger Leigh <rleigh@codelibre.net>

#=============================================================================
# Copyright 2014 Roger Leigh <rleigh@codelibre.net>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

# Find include directory
find_path(INTL_INCLUDE_DIR
        NAMES "libintl.h"
        DOC "libintl include directory")
mark_as_advanced(INTL_INCLUDE_DIR)

# Find all INTL libraries
find_library(INTL_LIBRARY "intl"
        DOC "libintl libraries (if not in the C library)")
mark_as_advanced(INTL_LIBRARY)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Intl
        REQUIRED_VARS INTL_INCLUDE_DIR INTL_LIBRARY
        FAIL_MESSAGE "Failed to find Gettext libintl")

if (INTL_FOUND)
    set(INTL_INCLUDE_DIRS "${INTL_INCLUDE_DIR}")
    set(INTL_LIBRARIES "${INTL_LIBRARY}")
endif ()
