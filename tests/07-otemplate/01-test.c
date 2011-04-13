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

#include <stdio.h>
#include <string.h>
#include <signal.h>

#include <onion/onion.h>
#include <onion/dict.h>
#include <onion/log.h>

onion *o;

int work(onion_dict *d, onion_request *req);

void free_onion(){
	ONION_INFO("Closing connections");
	onion_free(o);
	exit(0);
}

onion_connection_status test_page(void *ignore, onion_request *req){
	onion_dict *dict=onion_dict_new();
	
	char tmp[16];
	snprintf(tmp, sizeof(tmp), "%d", rand());
	
	onion_dict_add(dict, "title", "Test page - works", 0);
	onion_dict_add(dict, "random", tmp, OD_DUP_VALUE);
	
	int ok=work(dict, req);
	onion_dict_free(dict);
	return ok;
}

int main(int argc, char **argv){
	o=onion_new(O_ONE_LOOP);
	
	onion_handler *root=onion_handler_new((onion_handler_handler)test_page, NULL, NULL);

	onion_set_root_handler(o, root);
	signal(SIGINT, free_onion);
	
	onion_listen(o);
	
	return 0;
}
