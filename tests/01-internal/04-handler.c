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
#include <string.h>

#include <onion_dict.h>
#include <onion_server.h>
#include <onion_request.h>
#include <onion_response.h>
#include <onion_handler.h>
#include <onion_handler_static.h>
#include <onion_handler_path.h>

#include "../test.h"

void t01_handle_static_request(){
	INIT_LOCAL();

	char buffer[4096];
	char ok;
	memset(buffer,0,sizeof(buffer));
	onion_server *server=onion_server_new();
	onion_server_set_write(server, (onion_write)strncat);
	
	onion_request *request=onion_request_new(server, buffer);
	onion_request_fill(request,"GET / HTTP/1.1");
	
	onion_handler *handler=onion_handler_static(NULL, "Not ready",302);
	FAIL_IF_NOT(handler);
	onion_server_set_root_handler(server, handler);
	
	ok=onion_handler_handle(handler, request);
	FAIL_IF_NOT(ok);
	FAIL_IF_NOT_EQUAL_STR(buffer, "HTTP/1.1 302 REDIRECT\nContent-Length: 9\nServer: Onion lib - 0.1. http://coralbits.com\n\nNot ready");
	
	onion_request_free(request);
	onion_server_free(server);
	
	END_LOCAL();
}

void t02_handle_generic_request(){
	INIT_LOCAL();

	char buffer[4096];
	memset(buffer,0,sizeof(buffer));
	onion_server *server=onion_server_new();
	onion_server_set_write(server, (onion_write)strncat);
	
	onion_handler *handler=onion_handler_static("^/$", "Not ready",302);
	FAIL_IF_NOT(handler);

	onion_handler *handler2=onion_handler_static("^/a.*$", "any",200);
	FAIL_IF_NOT(handler);

	onion_handler *handler3=onion_handler_static(NULL, "Internal error",500);
	FAIL_IF_NOT(handler);

	onion_handler_add(handler, handler2);
	onion_handler_add(handler, handler3);
	onion_server_set_root_handler(server, handler);
	
	onion_request *request;

	request=onion_request_new(server, buffer);
	onion_request_fill(request,"GET / HTTP/1.1");
	onion_handler_handle(handler, request);
	FAIL_IF_NOT_EQUAL_STR(buffer, "HTTP/1.1 302 REDIRECT\nContent-Length: 9\nServer: Onion lib - 0.1. http://coralbits.com\n\nNot ready");
	onion_request_free(request);
	
	// gives error, as such url does not exist.
	memset(buffer,0,sizeof(buffer));
	request=onion_request_new(server, buffer);
	onion_request_fill(request,"GET /error HTTP/1.1");
	onion_handler_handle(handler, request);
	FAIL_IF_NOT_EQUAL_STR(buffer, "HTTP/1.1 500 INTERNAL ERROR\nContent-Length: 14\nServer: Onion lib - 0.1. http://coralbits.com\n\nInternal error");
	onion_request_free(request);

	memset(buffer,0,sizeof(buffer));
	request=onion_request_new(server, buffer);
	onion_request_fill(request,"GET /any HTTP/1.1");
	onion_handler_handle(handler, request);
	FAIL_IF_NOT_EQUAL_STR(buffer, "HTTP/1.1 200 OK\nContent-Length: 3\nServer: Onion lib - 0.1. http://coralbits.com\n\nany");
	onion_request_free(request);

	onion_server_free(server);

	END_LOCAL();
}

void t03_handle_path_request(){
	INIT_LOCAL();

	char buffer[4096];
	memset(buffer,0,sizeof(buffer));
	onion_server *server=onion_server_new();
	onion_server_set_write(server, (onion_write)strncat);

	onion_handler *test=onion_handler_static("^$", "Test index\n",200); // empty
	onion_handler_add(test, onion_handler_static("index.html", "Index test", 200) );
	
	onion_handler *path=onion_handler_path("^/test/", test);
	onion_handler_add(path, onion_handler_static(NULL, "Internal error", 500 ) );
	onion_server_set_root_handler(server, path);
	
	onion_request *request;
	request=onion_request_new(server, buffer);
	onion_request_fill(request,"GET / HTTP/1.1");
	onion_handler_handle(path, request);
	FAIL_IF_NOT_EQUAL_STR(buffer, "HTTP/1.1 500 INTERNAL ERROR\nContent-Length: 14\nServer: Onion lib - 0.1. http://coralbits.com\n\nInternal error");
	onion_request_free(request);
	
	// gives error, as such url does not exist.
	memset(buffer,0,sizeof(buffer));
	request=onion_request_new(server, buffer);
	onion_request_fill(request,"GET /test/ HTTP/1.1");
	onion_handler_handle(path, request);
	FAIL_IF_NOT_EQUAL_STR(buffer, "HTTP/1.1 200 OK\nContent-Length: 11\nServer: Onion lib - 0.1. http://coralbits.com\n\nTest index\n");
	onion_request_free(request);

	memset(buffer,0,sizeof(buffer));
	request=onion_request_new(server, buffer);
	onion_request_fill(request,"GET /test/index.html HTTP/1.1");
	onion_handler_handle(path, request);
	FAIL_IF_NOT_EQUAL_STR(buffer, "HTTP/1.1 200 OK\nContent-Length: 10\nServer: Onion lib - 0.1. http://coralbits.com\n\nIndex test");
	onion_request_free(request);

	onion_server_free(server);

	END_LOCAL();
}



int main(int argc, char **argv){
	t01_handle_static_request();
	t02_handle_generic_request();
	t03_handle_path_request();
	
	END();
}

