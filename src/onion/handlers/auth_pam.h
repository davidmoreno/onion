/*
	Onion HTTP server library
	Copyright (C) 2010 David Moreno Montero

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 3.0 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not see <http://www.gnu.org/licenses/>.
	*/

#ifndef __ONION_HANDLER_AUTH_PAM__
#define __ONION_HANDLER_AUTH_PAM__

#include <onion/types.h>

#ifdef __cplusplus
extern "C"{
#endif

/// Creates an auth handler that do not allow to pass unless user is authenticated using a pam name.
onion_handler *onion_handler_auth_pam(const char *realm, const char *pamname, onion_handler *inside_level);

#ifdef __cplusplus
}
#endif

#endif
