/*
	Onion HTTP server library
	Copyright (C) 2010 David Moreno Montero

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

#ifndef __ONION_SERVER__
#define __ONION_SERVER__

#include "onion_types.h"

#ifdef __cplusplus
extern "C"{
#endif

/// creates a new onion server
onion_server *onion_server_new();

/// frees the server.
void onion_server_free(onion_server *server);

/// Sets the write function
void onion_server_set_write(onion_server *server, onion_write write);

/// Sets the root handler
void onion_server_set_root_handler(onion_server *server, onion_handler *handler);

/// Handles a request
int onion_server_handle_request(onion_request *req);

#ifdef __cplusplus
}
#endif

#endif