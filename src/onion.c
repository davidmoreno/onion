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

#include <malloc.h>

#include "onion.h"
#include "onion_handler.h"

/// Creates the onion structure to fill with the server data, and later do the onion_listen()
onion *onion_new(int flags){
	onion *o=malloc(sizeof(onion));
	o->flags=flags;
	o->listenfd=0;
	o->root_handler=NULL;
	
	return o;
}

/// Performs the listening with the given mode
int onion_listen(onion *server){
	return 1;
}

/// Removes the allocated data
void onion_free(onion *onion){
	if (onion->root_handler)
		onion_handler_free(onion->root_handler);
	free(onion);
}

/// Sets the root handler
void onion_set_root_handler(onion *onion, onion_handler *handler){
	onion->root_handler=handler;
}
