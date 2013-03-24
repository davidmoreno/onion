/*
	Onion HTTP server library
	Copyright (C) 2010-2013 David Moreno Montero

	This library is free software; you can redistribute it and/or
	modify it under the terms of, at your choice:
	
	a. the GNU Lesser General Public License as published by the 
	 Free Software Foundation; either version 3.0 of the License, 
	 or (at your option) any later version.
	
	b. the GNU General Public License as published by the 
	 Free Software Foundation; either version 2.0 of the License, 
	 or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License and the GNU General Public License along with this 
	library; if not see <http://www.gnu.org/licenses/>.
	*/

#ifndef __ONION_SESSIONS_H__
#define __ONION_SESSIONS_H__

#include "types.h"

#ifdef __cplusplus
extern "C"{
#endif

/// Initializes the sessions object
onion_sessions *onion_sessions_new();

/// Frees the sessions object
void onion_sessions_free(onion_sessions *sessions);

/// Creates a session. Returns the session name. 
char *onion_sessions_create(onion_sessions *sessions);

/// Returns the session object.
onion_dict *onion_sessions_get(onion_sessions *sessions, const char *sessionId);

/// Removes a session from the storage.
void onion_sessions_remove(onion_sessions *sessions, const char *sessionId);

#ifdef __cplusplus
}
#endif

#endif
