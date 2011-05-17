/*
	Onion HTTP server library
	Copyright (C) 2010-2011 David Moreno Montero

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Affero General Public License as
	published by the Free Software Foundation, either version 3 of the
	License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Affero General Public License for more details.

	You should have received a copy of the GNU Affero General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
	*/

#ifndef __ONION_SHORTCUTS__
#define __ONION_SHORTCUTS__

#ifdef __cplusplus
extern "C"{
#endif

#include "types.h"

/// Shortcut for fast responses, like errors.
int onion_shortcut_response(const char *response, int code, onion_request *req, onion_response *res);

/// Shortcut for fast responses, like errors, with extra headers.
int onion_shortcut_response_extra_headers(const char *response, int code, onion_request *req, onion_response *res, ...);

/// Shortcut for fast redirect.
int onion_shortcut_redirect(const char *newurl, onion_request *req, onion_response *res);

/// Shortcut for response a static file on disk
int onion_shortcut_response_file(const char *filename, onion_request *req, onion_response *res);

/// Shortcut for response json data. Dict is freed before return.
int onion_shortcut_response_json(onion_dict *d, onion_request *req, onion_response *res);

#ifdef __cplusplus
}
#endif

#endif
