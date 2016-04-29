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

#ifndef ONION_SHORTCUTS_H
#define ONION_SHORTCUTS_H

#ifdef __cplusplus
extern "C"{
#endif

#include <time.h>

#include "types.h"

struct stat;

/// Shortcut for fast responses, like errors.
onion_connection_status onion_shortcut_response(const char *response, int code, onion_request *req, onion_response *res);

/// Shortcut for fast responses, like errors, with extra headers.
onion_connection_status onion_shortcut_response_extra_headers(const char *response, int code, onion_request *req, onion_response *res, ...);

/// Shortcut for fast redirect. It tells the browser to look somewhere else.
onion_connection_status onion_shortcut_redirect(const char *newurl, onion_request *req, onion_response *res);

/// Shortcut for fast internal redirect. It returns what the server would return with the new address.
onion_connection_status onion_shortcut_internal_redirect(const char *newurl, onion_request *req, onion_response *res);

/// Shortcut for response a static file on disk
onion_connection_status onion_shortcut_response_file(const char *filename, onion_request *req, onion_response *res);

/// Shortcut for response json data. Dict is freed before return.
onion_connection_status onion_shortcut_response_json(onion_dict *d, onion_request *req, onion_response *res);

/// Shortcut to return the date in "RFC 822 / section 5, 4 digit years" date format. 
void onion_shortcut_date_string(time_t t, char *dest);
/// Shortcut to return the date in ISO format
void onion_shortcut_date_string_iso(time_t t, char *dest);

/// Shortcut to return the date in time_t from a user given date TODO
//time_t onion_shortcut_date_time_t(const char *t);

/// Shortcut to unify the creation of etags.
void onion_shortcut_etag(struct stat *, char etag[32]);
/// Moves a file to another location
int onion_shortcut_rename(const char *orig, const char *dest);


#ifdef __cplusplus
}
#endif

#endif
