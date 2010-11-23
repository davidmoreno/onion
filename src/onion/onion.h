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

#ifndef __ONION__
#define __ONION__

#include "onion_types.h"

#ifdef __cplusplus
extern "C"{
#endif

/// Creates the onion structure to fill with the server data, and later do the onion_listen()
onion *onion_new(int flags);

/// Performs the listening with the given mode
int onion_listen(onion *server);

/// Removes the allocated data
void onion_free(onion *onion);

/// Sets the root handler
void onion_set_root_handler(onion *server, onion_handler *handler);

/// Sets the port to listen
void onion_set_port(onion *server, int port);

/// Set a certificate for use in the connection
int onion_use_certificate(onion *onion, onion_ssl_certificate_type type, const char *filename, ...);

/// Gets the current flags, for example to check SSL support.
int onion_flags(onion *onion);

/// Sets the timeout, in milliseconds, 0 dont wait for incomming data (too strict maybe), -1 forever, clients closes connection
void onion_set_timeout(onion *onion, int timeout);

/// Sets the maximum number of threads to use for requests. default 16.
void onion_set_max_threads(onion *onion, int max_threads);

#ifdef __cplusplus
}
#endif


#endif
