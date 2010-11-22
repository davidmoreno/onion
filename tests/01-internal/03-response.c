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

#include <onion/onion_dict.h>
#include <onion/onion_server.h>
#include <onion/onion_request.h>
#include <onion/onion_response.h>
#include <onion/onion_types_internal.h>

#include "../test.h"

void t01_create_add_free(){
	INIT_LOCAL();
	
	onion_response *res;
	
	res=onion_response_new(NULL);
	FAIL_IF_NOT_EQUAL(res->code, 200);
	
	FAIL_IF_EQUAL(res,NULL);
	
	onion_response_set_length(res,1024);
	FAIL_IF_NOT_EQUAL_STR(onion_dict_get(res->headers,"Content-Length"),"1024");
	
	onion_response_free(res);
	
	END_LOCAL();
}

/// Just appends to the handler. Must be big enought or segfault.. Just for tests.
int write_append(void *handler, const char *data, unsigned int length){
	char *str=handler;
	int p=strlen(str);
	strcat(str,data);
	str[p+length]=0;
	return length;
}


void t02_full_cycle(){
	INIT_LOCAL();
	
	onion_server *server=onion_server_new();
	onion_server_set_write(server, write_append);
	onion_request *request;
	char buffer[4096];
	memset(buffer,0,sizeof(buffer));
	
	request=onion_request_new(server, buffer, NULL);
	
	onion_response *response=onion_response_new(request);
	
	onion_response_set_length(response, 30);
	FAIL_IF_NOT_EQUAL(response->length,30);
	onion_response_write_headers(response);
	
	onion_response_write0(response,"123456789012345678901234567890");
	
	FAIL_IF_NOT_EQUAL(response->sent_bytes,30);
	
	onion_response_free(response);
	onion_request_free(request);
	onion_server_free(server);
	
	FAIL_IF_NOT_EQUAL_STR(buffer, "HTTP/1.1 200 OK\nContent-Length: 30\nServer: Onion lib - 0.1. http://coralbits.com\n\n123456789012345678901234567890");
	
	END_LOCAL();
}

int main(int argc, char **argv){
	t01_create_add_free();
	t02_full_cycle();
	
	END();
}

