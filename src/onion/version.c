/*
	Onion HTTP server library
	Copyright (C) 2010-2016 David Moreno Montero and others

	This library is free software; you can redistribute it and/or
	modify it under the terms of, at your choice:

	a. the Apache License Version 2.0.

	b. the GNU General Public License as published by the
		Free Software Foundation; either version 2.0 of the License,
		or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of both libraries, if not see
	<http://www.gnu.org/licenses/> and
	<http://www.apache.org/licenses/LICENSE-2.0>.
	*/

#include <onion/log.h>
#include <onion/version.h>


/**
 * @defgroup version Version. Onion Semantic Versioning
 * @{
 * @shortname Onion runtime version control
 *
 * This version control functions can be used to control if current runtime
 * is compatible with the onion library in use.
 *
 * Normal use is call the macro:
 *
 * @code
 *  ONION_VERSION_IS_COMPATIBLE_OR_ABORT();
 * @endcode
 *
 * at the begining of your program, which will ensure the current running
 * program was compiled with a compatible onion version to the runtime one,
 * or will call abort()
 *
 */

/**
 * @short Current running onion version as a string
 */
const char *onion_version(){
  return ONION_VERSION;
}


/**
 * @short Current running onion major version
 */
int onion_version_major(){
  return ONION_VERSION_MAJOR;
}

/**
 * @short Current running onion minor version
 */
int onion_version_minor(){
  return ONION_VERSION_MINOR;
}
/**
 * @short Patch version of the current release
 *
 * If on a dirty git version (not tagged), it will return 1000
 */
int onion_version_patch(){
  return ONION_VERSION_PATCH;
}

/**
 * @short Checks a specific set of major.minor.patch and returns if the current using onion is ABI compatible.
 *
 * Onion uses SEMVER (http://semver.org/), and with this simple function its
 * possible to check if your compiled code is compatible with the onion
 * version.
 *
 * It also allows to in the rare case that there is some really bad version of
 * onion to warn the users.
 *
 * Normally users need just to add a onion_version_is_compatible() check, and
 * if not compatible abort:
 *
 *   if (!onion_version_is_compatible()) abort();
 *
 */
bool onion_version_is_compatible3(int major, int minor, int patch){
  if (major != ONION_VERSION_MAJOR){
    ONION_DEBUG("Onion major version (%d) is not compatible with program's (%d). Should match.", ONION_VERSION_MAJOR, major);
    return false;
  }
  if (minor > ONION_VERSION_MINOR){
    ONION_DEBUG("Onion minor version (%d) is not compatible with program's (%d). Program's has to be equal or greater.", ONION_VERSION_MINOR, minor);
    return false;
  }
  return true;
}

// This is to ensure documentation generation. Will never be compiled. Check header for real. implementation.
#ifndef __ONION_VERSION_H__
/// Macro to ensure that the version of onion your program is using is ABI compatible with the library one
#define ONION_VERSION_IS_COMPATIBLE()

/// Macro to ensure that the version of onion your program is using is ABI compatible with the library one. It aborts if its not compatible.
#define ONION_VERSION_IS_COMPATIBLE_OR_ABORT()

#endif

/// @}
