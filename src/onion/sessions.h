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

#ifndef ONION_SESSIONS_H
#define ONION_SESSIONS_H

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

/// Store session
void onion_sessions_save(onion_sessions *sessions, const char *sessionId, onion_dict *data);

/// Removes a session from the storage.
void onion_sessions_remove(onion_sessions *sessions, const char *sessionId);

#ifdef __cplusplus
}
#endif

#endif
