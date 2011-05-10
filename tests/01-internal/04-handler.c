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
#include <stdlib.h>

#include <onion/dict.h>
#include <onion/server.h>
#include <onion/request.h>
#include <onion/response.h>
#include <onion/handler.h>
#include <onion/log.h>

#include <onion/handlers/static.h>
#include <onion/handlers/path.h>

#include "../test.h"

#define FILL(a,b) onion_request_write(a,b,strlen(b))

ssize_t mstrncat(char *a, const char *b, size_t l){
	strncat(a,b,l);
	ONION_DEBUG("len %d",strlen(a));
	return strlen(a);
}

void t01_handle_static_request(){
	INIT_LOCAL();

	char buffer[4096];
	char ok;
	memset(buffer,0,sizeof(buffer));
	onion_server *server=onion_server_new();
	onion_server_set_write(server, (onion_write)mstrncat);
	
	onion_request *request=onion_request_new(server, buffer, NULL);
	FILL(request,"GET / HTTP/1.1\n");
	
	onion_handler *handler=onion_handler_static(NULL, "Not ready",302);
	FAIL_IF_NOT(handler);
	onion_server_set_root_handler(server, handler);
	
	onion_response *response=onion_response_new(request);
	ok=onion_handler_handle(handler, request, response);
	onion_response_free(response);
	FAIL_IF_NOT(ok);
	FAIL_IF_NOT_EQUAL_STR(buffer, "HTTP/1.1 302 REDIRECT\r\nContent-Length: 9\r\nServer: libonion v0.1 - coralbits.com\r\n\r\nNot ready");
	
	onion_request_free(request);
	onion_server_free(server);
	
	END_LOCAL();
}

void t02_handle_generic_request(){
	INIT_LOCAL();

	char buffer[4096];
	memset(buffer,0,sizeof(buffer));
	onion_server *server=onion_server_new();
	onion_server_set_write(server, (onion_write)mstrncat);
	
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
	onion_response *response;

	request=onion_request_new(server, buffer, NULL);
	FILL(request,"GET / HTTP/1.1\n");
	response=onion_response_new(request);
	onion_handler_handle(handler, request, response);
	onion_response_free(response);
	FAIL_IF_NOT_EQUAL_STR(buffer, "HTTP/1.1 302 REDIRECT\r\nContent-Length: 9\r\nServer: libonion v0.1 - coralbits.com\r\n\r\nNot ready");
	onion_request_free(request);
	
	// gives error, as such url does not exist.
	memset(buffer,0,sizeof(buffer));
	request=onion_request_new(server, buffer, "localhost");
	FILL(request,"GET /error HTTP/1.1\n");
	response=onion_response_new(request);
	onion_handler_handle(handler, request,response);
	onion_response_free(response);
	FAIL_IF_NOT_EQUAL_STR(buffer, "HTTP/1.1 500 INTERNAL ERROR\r\nContent-Length: 14\r\nServer: libonion v0.1 - coralbits.com\r\n\r\nInternal error");
	onion_request_free(request);

	memset(buffer,0,sizeof(buffer));
	request=onion_request_new(server, buffer, "127.0.0.1");
	FILL(request,"GET /any HTTP/1.1\n");
	response=onion_response_new(request);
	onion_handler_handle(handler, request,response);
	onion_response_free(response);
	FAIL_IF_NOT_EQUAL_STR(buffer, "HTTP/1.1 200 OK\r\nContent-Length: 3\r\nServer: libonion v0.1 - coralbits.com\r\n\r\nany");
	onion_request_free(request);

	onion_server_free(server);

	END_LOCAL();
}

void t03_handle_path_request(){
	INIT_LOCAL();

	char buffer[4096];
	memset(buffer,0,sizeof(buffer));
	onion_server *server=onion_server_new();
	onion_server_set_write(server, (onion_write)mstrncat);

	onion_handler *test=onion_handler_static("^$", "Test index\n",200); // empty
	onion_handler_add(test, onion_handler_static("index.html", "Index test", 200) );
	
	onion_handler *path=onion_handler_path("^/test/", test);
	onion_handler_add(path, onion_handler_static(NULL, "Internal error", 500 ) );
	onion_server_set_root_handler(server, path);
	
	onion_request *request;
	onion_response *response;
	
	request=onion_request_new(server, buffer, NULL);
	FILL(request,"GET / HTTP/1.1\n");
	response=onion_response_new(request);
	onion_handler_handle(path, request, response);
	onion_response_free(response);
	FAIL_IF_NOT_EQUAL_STR(buffer, "HTTP/1.1 500 INTERNAL ERROR\r\nContent-Length: 14\r\nServer: libonion v0.1 - coralbits.com\r\n\r\nInternal error");
	onion_request_free(request);
	
	// gives error, as such url does not exist.
	memset(buffer,0,sizeof(buffer));
	request=onion_request_new(server, buffer, NULL);
	FILL(request,"GET /test/ HTTP/1.1\n");
	response=onion_response_new(request);
	onion_handler_handle(path, request, response);
	onion_response_free(response);
	FAIL_IF_NOT_EQUAL_STR(buffer, "HTTP/1.1 200 OK\r\nContent-Length: 11\r\nServer: libonion v0.1 - coralbits.com\r\n\r\nTest index\n");
	onion_request_free(request);

	memset(buffer,0,sizeof(buffer));
	request=onion_request_new(server, buffer, NULL);
	FILL(request,"GET /test/index.html HTTP/1.1\n");
	response=onion_response_new(request);
	onion_handler_handle(path, request, response);
	onion_response_free(response);
	FAIL_IF_NOT_EQUAL_STR(buffer, "HTTP/1.1 200 OK\r\nContent-Length: 10\r\nServer: libonion v0.1 - coralbits.com\r\n\r\nIndex test");
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

