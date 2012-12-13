/*
	Onion HTTP server library
	Copyright (C) 2010-2011 David Moreno Montero

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
#include <onion/handlers/exportlocal.h>
#include <onion/log.h>
#include <onion/block.h>

#include <string.h>
#include <unistd.h>

#include "../ctest.h"
#include </home/dmoreno/src/onion/tests/01-internal/buffer_listen_point.h>

onion *server;

int GET(onion_request *req, const char *url){
	onion_request_clean(req);
	char tmp[1024];
	snprintf(tmp, sizeof(tmp), "GET %s HTTP/1.1\n\n", url);
	ONION_DEBUG("Request: %s",tmp);
	
	return onion_request_write(req, tmp, strlen(tmp));
}

void t01_exportdir(){
	INIT_TEST();
	
	int code;
	
	onion_handler *handler=onion_handler_export_local_new(".");
	onion_set_root_handler(server, handler);
	
	onion_request *req=onion_request_new(onion_get_listen_point(server, 0));
	code=GET(req, "/02-exportlocal");
	FAIL_IF_NOT_EQUAL_INT(code, OCS_KEEP_ALIVE);
	onion_block *block=onion_buffer_listen_point_get_buffer(req);
	FAIL_IF_EQUAL_INT((int)server_buffer->pos, 0);
	int olds=onion_block_size(block);

	code=GET(req, "/");
	FAIL_IF_EQUAL_INT(code, 0);
	FAIL_IF_EQUAL_INT(onion_block_size(block), olds);

	onion_request_free(req);
	onion_handler_free(handler);
	onion_set_root_handler(server, NULL);
	
	END_TEST();
}

void t02_exportfile(){
	INIT_TEST();
	
	int code;
	
	onion_handler *handler=onion_handler_export_local_new("./02-exportlocal");
	onion_server_set_root_handler(server, handler);
	
	onion_request *req=onion_request_new(server, server_buffer, "TEST");
	code=GET(req, "/02-exportlocal");
	FAIL_IF_NOT_EQUAL_INT(code, OCS_KEEP_ALIVE);
	FAIL_IF_EQUAL_INT((int)server_buffer->pos, 0);
	int olds=server_buffer->pos;

	code=GET(req, "/");
	FAIL_IF_NOT_EQUAL_INT(code, OCS_KEEP_ALIVE);
	FAIL_IF_NOT_EQUAL_INT((int)server_buffer->pos, olds);

	onion_request_free(req);
	onion_handler_free(handler);
	onion_server_set_root_handler(server, NULL);
	
	END_TEST();
}

void init(){
	server=onion_new(0);
	onion_add_listen_point(server, onion_buffer_listen_point_new());
}

void end(){
	onion_free(server);
}

int main(int argc, char **argv){
	if (chdir(dirname(argv[0]))!=0){
		ONION_ERROR("Error changing dir to test dir");
		exit(1);
	}
	init();

	t01_exportdir();
	t02_exportfile();
	
	end();
	END();
}
