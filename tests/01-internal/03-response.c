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

#include <onion_response.h>

#include "../test.h"

void t01_create_add_free(){
	INIT_LOCAL();
	
	onion_response *req;
	
	req=onion_response_new();
	FAIL_IF_NOT_EQUAL(req->code, 200);
	
	FAIL_IF_EQUAL(req,NULL);
	
	onion_response_set_length(req,1024);
	FAIL_IF_NOT_EQUAL_STR(onion_dict_get(req->headers,"Content-Length"),"1024");
	
	onion_response_free(req);
	
	END_LOCAL();
}
/*
void t02_create_add_free_overflow(){
	INIT_LOCAL();
	
	onion_request *req;
	int ok, i;
	
	req=onion_request_new(12);
	FAIL_IF_NOT_EQUAL(req->socket, 12);

	FAIL_IF_EQUAL(req,NULL);
	
	char of[4096];
	for (i=0;i<sizeof(of);i++)
		of[i]='a'+i%26;
	char get[4096*4];
	sprintf(get,"%s %s %s",of,of,of);
	ok=onion_request_fill(req,get);
	FAIL_IF_NOT_EQUAL(ok,0);
	
	onion_request_free(req);
	
	END_LOCAL();
}

void t03_create_add_free_full_flow(){
	INIT_LOCAL();
	
	onion_request *req;
	int ok;
	
	req=onion_request_new(0);
	FAIL_IF_EQUAL(req,NULL);
	FAIL_IF_NOT_EQUAL(req->socket, 0);
	
	ok=onion_request_fill(req,"GET /myurl%20/is/very/deeply/nested?test=test&query2=query%202&more_query=%20more%20query+10 HTTP/1.1\n");
	FAIL_IF_NOT(ok);
	ok=onion_request_fill(req,"Host: 127.0.0.1");
	FAIL_IF_NOT(ok);
	ok=onion_request_fill(req,"Other-Header: My header is very long and with spaces...\n");
	FAIL_IF_NOT(ok);
	
	FAIL_IF_NOT_EQUAL(req->flags,OR_GET|OR_HTTP11);
	
	FAIL_IF_EQUAL(req->headers, NULL);
	FAIL_IF_NOT_EQUAL_STR( onion_dict_get(req->headers,"Host"), "127.0.0.1");
	FAIL_IF_NOT_EQUAL_STR( onion_dict_get(req->headers,"Other-Header"), "My header is very long and with spaces...");

	onion_request_parse_query(req);
	
	FAIL_IF_NOT_EQUAL_STR(req->url,"/myurl /is/very/deeply/nested");

	FAIL_IF_EQUAL(req->query,NULL);
	FAIL_IF_NOT_EQUAL_STR( onion_dict_get(req->query,"test"), "test");
	FAIL_IF_NOT_EQUAL_STR( onion_dict_get(req->query,"query2"), "query 2");
	FAIL_IF_NOT_EQUAL_STR( onion_dict_get(req->query,"more_query"), " more query 10");
	
	
	onion_request_free(req);
	
	END_LOCAL();
}

*/
int main(int argc, char **argv){
	t01_create_add_free();
	
	END();
}

