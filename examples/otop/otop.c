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

#include <string.h>
#include <stdio.h>

#include <onion.h>
#include <onion_handler.h>
#include <handlers/onion_handler_directory.h>
#include <handlers/onion_handler_static.h>

onion_handler *otop_handler_new();

/**
 * @short Exports a top like structure
 */
int main(int argc, char **argv){
	onion_handler *otop=otop_handler_new();
	
	onion *onion=onion_new(O_ONE);
	onion_set_root_handler(onion, otop);
	onion_set_port(onion, 8080);
	
	int error=onion_listen(onion);
	if (error){
		perror("Cant create the server");
	}
	
	onion_free(onion);
	
	return 0;
}

/// The handler created here.
int otop_handler(void *d, onion_request *req){
	return 0;
}

/**
 * @short Creates a top like handler.
 */
onion_handler *otop_handler_new(){
	onion_handler *otop=onion_handler_new((onion_handler_handler)otop_handler, NULL, NULL);
	
	return otop;
}
