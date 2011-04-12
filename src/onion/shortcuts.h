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

#include <onion/types.h>

/// Shortcut for fast responses, like errors.
int onion_shortcut_response(onion_request *req, const char *response, int code);

/// Shortcut for fast responses, like errors, with extra headers.
int onion_shortcut_response_extra_headers(onion_request *req, const char *response, int code, ...);

/// Shortcut for fast redirect.
int onion_shortcut_redirect(onion_request *req, const char *response, const char *newurl);


#ifdef __cplusplus
}
#endif

#endif
