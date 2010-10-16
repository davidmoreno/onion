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

#include "onion_server.h"
#include "onion_handler.h"
#include "onion_types_internal.h"

onion_server *onion_server_new(void){
	onion_server *ret=malloc(sizeof(onion_server));
	ret->root_handler=NULL;
	ret->write=NULL;
	return ret;
}

void onion_server_free(onion_server *server){
	if (server->root_handler)
		onion_handler_free(server->root_handler);
	free(server);
}

void onion_server_set_write(onion_server *server, onion_write write){
	server->write=write;
}

void onion_server_set_root_handler(onion_server *server, onion_handler *handler){
	server->root_handler=handler;
}
