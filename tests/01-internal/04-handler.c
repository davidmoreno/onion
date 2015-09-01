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
#include <stdlib.h>

#include <onion/onion.h>
#include <onion/dict.h>
#include <onion/request.h>
#include <onion/response.h>
#include <onion/handler.h>
#include <onion/log.h>

#include <onion/handlers/static.h>
#include <onion/handlers/path.h>

#include "../ctest.h"
#include "buffer_listen_point.h"
#include <onion/url.h>

#define FILL(a,b) onion_request_write(a,b,strlen(b))

void t01_handle_static_request(){
	INIT_LOCAL();

	char ok;
	onion *server=onion_new(0);
	onion_listen_point *lp=onion_buffer_listen_point_new();
	onion_add_listen_point(server, NULL, NULL, lp);

	onion_request *request=onion_request_new(lp);
	FILL(request,"GET / HTTP/1.1\n");

	onion_handler *handler=onion_handler_static("Not ready",302);
	FAIL_IF_NOT(handler);
	onion_set_root_handler(server, handler);

	onion_response *response=onion_response_new(request);
	ok=onion_handler_handle(handler, request, response);
	FAIL_IF_NOT_EQUAL(ok, OCS_PROCESSED);
	onion_response_free(response);

	const char *buffer=onion_buffer_listen_point_get_buffer_data(request);
	FAIL_IF_EQUAL_STR(buffer,"");
	FAIL_IF_NOT_STRSTR(buffer, "HTTP/1.1 302 Found\r\n");
	FAIL_IF_NOT_STRSTR(buffer, "\r\nContent-Length: 9\r\n");
	FAIL_IF_NOT_STRSTR(buffer, "libonion");
	FAIL_IF_STRSTR(buffer, "License: AGPL"); // License is now LGPL, no need to advertise
	FAIL_IF_STRSTR(buffer, "License");
	FAIL_IF_NOT_STRSTR(buffer, "\r\n\r\nNot ready");

	onion_request_free(request);
	onion_free(server);

	END_LOCAL();
}

void t02_handle_generic_request(){
	INIT_LOCAL();

	onion *server=onion_new(0);
	onion_listen_point *lp=onion_buffer_listen_point_new();
	onion_add_listen_point(server, NULL, NULL, lp);

	onion_url *url=onion_url_new();
	int error;
	error=onion_url_add_handler(url, "^$", onion_handler_static("Not ready",302));
	FAIL_IF(error);

	error=onion_url_add_handler(url, "^a.*$", onion_handler_static("any",200));
	FAIL_IF(error);

	error=onion_url_add_static(url, "^.*", "Internal error", 500);
	FAIL_IF(error);

	onion_set_root_handler(server, onion_url_to_handler(url));

	onion_request *request;


	request=onion_request_new(lp);
	FILL(request,"GET / HTTP/1.1\n");
	onion_request_process(request);
	const char *buffer=onion_buffer_listen_point_get_buffer_data(request);
	FAIL_IF_NOT_STRSTR(buffer, "HTTP/1.1 302 Found\r\n");
	FAIL_IF_NOT_STRSTR(buffer, "Content-Length: 9\r\n");
	FAIL_IF_NOT_STRSTR(buffer, "\r\n\r\nNot ready");
	onion_request_free(request);

	// gives error, as such url does not exist.
	request=onion_request_new(lp);
	FILL(request,"GET /error HTTP/1.1\n");
	onion_request_process(request);
	buffer=onion_buffer_listen_point_get_buffer_data(request);
	ONION_DEBUG("<%s>", buffer);
	FAIL_IF_NOT_STRSTR(buffer, "HTTP/1.1 500 Internal Server Error\r\n");
	FAIL_IF_NOT_STRSTR(buffer, "Content-Length: 14\r\n");
	FAIL_IF_NOT_STRSTR(buffer, "\r\n\r\nInternal error");
	onion_request_free(request);

	request=onion_request_new(lp);
	FILL(request,"GET /any HTTP/1.1\n");
	onion_request_process(request);
	buffer=onion_buffer_listen_point_get_buffer_data(request);
	FAIL_IF_NOT_STRSTR(buffer, "HTTP/1.1 200 OK\r\n");
	FAIL_IF_NOT_STRSTR(buffer, "Content-Length: 3\r\n");
	FAIL_IF_NOT_STRSTR(buffer, "\r\n\r\nany");
	onion_request_free(request);

	onion_free(server);

	END_LOCAL();
}

void t03_handle_path_request(){
	INIT_LOCAL();

	onion *server=onion_new(0);
	onion_listen_point *lp=onion_buffer_listen_point_new();
	onion_add_listen_point(server, NULL, NULL, lp);

	onion_url *urls=onion_url_new();

	onion_url_add_static(urls, "^$", "Test index\n", HTTP_OK);
	onion_url_add_static(urls, "^index.html$", "Index test", 200);

	onion_url *pathu=onion_url_new();
	onion_handler *path=onion_url_to_handler(pathu);
	onion_url_add_url(pathu, "^test/", urls);
	onion_handler_add(path, onion_handler_static("Internal error", 500 ) );
	onion_set_root_handler(server, path);

	onion_request *request;
	onion_response *response;

	request=onion_request_new(lp);
	FILL(request,"GET / HTTP/1.1\n");
  onion_request_polish(request);
	response=onion_response_new(request);
	onion_handler_handle(path, request, response);
	onion_response_free(response);
	const char *buffer=onion_buffer_listen_point_get_buffer_data(request);
	FAIL_IF_NOT_STRSTR(buffer, "HTTP/1.1 500 Internal Server Error\r\n");
	FAIL_IF_NOT_STRSTR(buffer, "Content-Length: 14\r\n");
	FAIL_IF_NOT_STRSTR(buffer, "\r\n\r\nInternal error");
	onion_request_free(request);

	// gives error, as such url does not exist.
	request=onion_request_new(lp);
	FILL(request,"GET /test/ HTTP/1.1\n");
  onion_request_polish(request);
	response=onion_response_new(request);
	onion_handler_handle(path, request, response);
	onion_response_free(response);
	buffer=onion_buffer_listen_point_get_buffer_data(request);
	FAIL_IF_NOT_STRSTR(buffer, "HTTP/1.1 200 OK\r\n");
	FAIL_IF_NOT_STRSTR(buffer, "Content-Length: 11\r\n");
	FAIL_IF_NOT_STRSTR(buffer, "\r\n\r\nTest index\n");
	onion_request_free(request);

	request=onion_request_new(lp);
	FILL(request,"GET /test/index.html HTTP/1.1\n");
  onion_request_polish(request);
	response=onion_response_new(request);
	onion_handler_handle(path, request, response);
	onion_response_free(response);
	buffer=onion_buffer_listen_point_get_buffer_data(request);
	FAIL_IF_NOT_STRSTR(buffer, "HTTP/1.1 200 OK\r\n");
	FAIL_IF_NOT_STRSTR(buffer, "Content-Length: 10\r\n");
	FAIL_IF_NOT_STRSTR(buffer, "\r\n\r\nIndex test");
	onion_request_free(request);

	onion_free(server);

	END_LOCAL();
}



int main(int argc, char **argv){
  START();

	t01_handle_static_request();
	t02_handle_generic_request();
	t03_handle_path_request();

	END();
}
