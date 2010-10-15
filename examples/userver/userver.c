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

#include <onion.h>
#include <onion_handler.h>
#include <handlers/onion_handler_static.h>

int main(int argc, char **argv){
	onion_handler *sttic=onion_handler_static(NULL,"Internal error", 500);
	
	onion *onion=onion_new(O_ONE);
	onion_set_root_handler(onion, sttic);
	
	onion_listen(onion);
	
	onion_free(onion);
	
	return 0;
}
