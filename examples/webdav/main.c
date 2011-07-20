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
#include <onion/onion.h>

#include "webdav.h"

int main(int argc, char **argv){
	onion *o=onion_new(O_THREADED);
	
	onion_url *urls=onion_root_url(o);
	onion_url_add_handler(urls, "^", onion_webdav("."));
	
	onion_listen(o);
	
	return 0;
}

