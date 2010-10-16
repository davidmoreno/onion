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

enum onion_mode_e{
	O_ONE=1,
	O_ONE_LOOP=3,
	O_THREADED=4
};

typedef enum onion_mode_e onion_mode;

typedef struct onion_t onion;

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

#ifdef __cplusplus
}
#endif


#endif
