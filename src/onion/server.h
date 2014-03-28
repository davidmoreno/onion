/*
	Onion HTTP server library
	Copyright (C) 2010-2014 David Moreno Montero and othes

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

#ifndef ONION_SERVER_H
#define ONION_SERVER_H

#include <stddef.h>

#include "types.h"

#ifdef __cplusplus
extern "C"{
#endif

/// creates a new onion server
onion_server *onion_server_new();

/// frees the server.
void onion_server_free(onion_server *server);
/// Sets the write function
void onion_server_set_write(onion_server *server, onion_write write);
/// Sets the close function
void onion_server_set_close(onion_server *server, onion_close close);

/// Sets the read function
void onion_server_set_read(onion_server *server, onion_read read);

/// Sets the root handler
void onion_server_set_root_handler(onion_server *server, onion_handler *handler);

/// Sets the root handler
void onion_server_set_internal_error_handler(onion_server *server, onion_handler *handler);

/// Handles a request
onion_connection_status onion_server_handle_request(onion_server *server, onion_request *req);

/// Sets the maximum post size
void onion_server_set_max_post_size(onion_server *server, size_t max_post_size);

/// Sets the maximum file size
void onion_server_set_max_file_size(onion_server *server, size_t max_file_size);

/// Writes some data to a specific request.
onion_connection_status onion_server_write_to_request(onion_server *server, onion_request *request, const char *data, size_t len);

/// Returns the onion struct for this server, if any
onion *onion_server_get_onion(onion_server *o);

#ifdef __cplusplus
}
#endif

#endif
