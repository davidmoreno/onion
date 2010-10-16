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
#include <unistd.h>

#include <onion_dict.h>
#include <onion_server.h>
#include <onion_request.h>
#include <onion_response.h>
#include <onion_handler.h>
#include <handlers/onion_handler_static.h>
#include <handlers/onion_handler_path.h>

#include "../test.h"

void t01_server_min(){
	INIT_LOCAL();
	char buffer[4096];
	memset(buffer,0,sizeof(buffer));
	
	onion_server server;
	server.write=(onion_write)strncat;
	server.root_handler=onion_handler_static("", "Succedded", 200);
	
	onion_request *req=onion_request_new(&server, buffer);
	onion_request_write(req, "GET ",4);
	onion_request_write(req, "/",1);
	onion_request_write(req, " HTTP/1.1\n",10);
	
	onion_request_write(req, "\n",1);
	
	FAIL_IF_EQUAL_STR(buffer,"");
	FAIL_IF_NOT_EQUAL_STR(buffer,"HTTP/1.1 200 OK\nContent-Length: 9\n\nSuccedded");
	
	onion_request_free(req);
	onion_handler_free(server.root_handler);
	
	END_LOCAL();
}

void t02_server_full(){
	INIT_LOCAL();
	char buffer[4096];
	memset(buffer,0,sizeof(buffer));
	
	onion_server server;
	server.write=(onion_write)strncat;
	server.root_handler=onion_handler_static("", "Succedded", 200);
	
	onion_request *req=onion_request_new(&server, buffer);
#define S "GET / HTTP/1.1\nHeader-1: This is header1\nHeader-2: This is header 2\n"
	onion_request_write(req, S,sizeof(S)-1); // send it all, but the final 0.
#undef S
	FAIL_IF_NOT_EQUAL_STR(buffer,"");
	onion_request_write(req, "\n",1); // finish this request. no \n\n before to check possible bugs.
	
	FAIL_IF_EQUAL_STR(buffer,"");
	FAIL_IF_NOT_EQUAL_STR(buffer,"HTTP/1.1 200 OK\nContent-Length: 9\n\nSuccedded");

	onion_request_free(req);
	onion_handler_free(server.root_handler);

	END_LOCAL();
}

void t03_server_no_overflow(){
	INIT_LOCAL();
	char buffer[4096];
	memset(buffer,0,sizeof(buffer));
	
	onion_server server;
	server.write=(onion_write)strncat;
	server.root_handler=onion_handler_static("", "Succedded", 200);
	
	onion_request *req=onion_request_new(&server, buffer);
#define S "GET / HTTP/1.1\nHeader-1: This is header1\nHeader-2: This is header 2\nHeader-1: This is header1\nHeader-2: This is header 2\nHeader-1: This is header1\nHeader-2: This is header 2\nHeader-1: This is header1\nHeader-2: This is header 2\nHeader-1: This is header1\nHeader-2: This is header 2\nHeader-1: This is header1\nHeader-2: This is header 2\nHeader-1: This is header1\nHeader-2: This is header 2\nHeader-1: This is header1\nHeader-2: This is header 2\nHeader-1: This is header1\nHeader-2: This is header 2\nHeader-1: This is header1\nHeader-2: This is header 2\nHeader-1: This is header1\nHeader-2: This is header 2\nHeader-1: This is header1\nHeader-2: This is header 2\nHeader-1: This is header1\nHeader-2: This is header 2\nHeader-1: This is header1\nHeader-2: This is header 2\nHeader-1: This is header1\nHeader-2: This is header 2\nHeader-1: This is header1\nHeader-2: This is header 2\nHeader-1: This is header1\nHeader-2: This is header 2\nHeader-1: This is header1\nHeader-2: This is header 2\nHeader-1: This is header1\nHeader-2: This is header 2\nHeader-1: This is header1\nHeader-2: This is header 2\nHeader-1: This is header1\nHeader-2: This is header 2\nHeader-1: This is header1\nHeader-2: This is header 2\nHeader-1: This is header1\nHeader-2: This is header 2\nHeader-1: This is header1\nHeader-2: This is header 2\nHeader-1: This is header1\nHeader-2: This is header 2\nHeader-1: This is header1\nHeader-2: This is header 2\nHeader-1: This is header1\nHeader-2: This is header 2\nHeader-1: This is header1\nHeader-2: This is header 2\nHeader-1: This is header1\nHeader-2: This is header 2\nHeader-1: This is header1\nHeader-2: This is header 2\nHeader-1: This is header1\nHeader-2: This is header 2\nHeader-1: This is header1\nHeader-2: This is header 2\n"
	onion_request_write(req, S,sizeof(S)-1); // send it all, but the final 0.
#undef S
	FAIL_IF_NOT_EQUAL_STR(buffer,"");
	onion_request_write(req, "\n",1); // finish this request. no \n\n before to check possible bugs.
	
	FAIL_IF_EQUAL_STR(buffer,"");
	FAIL_IF_NOT_EQUAL_STR(buffer,"HTTP/1.1 200 OK\nContent-Length: 9\n\nSuccedded");

	onion_request_free(req);
	onion_handler_free(server.root_handler);

	END_LOCAL();
}

void t04_server_overflow(){
	INIT_LOCAL();
	char buffer[4096];
	memset(buffer,0,sizeof(buffer));
	
	onion_server server;
	server.write=(onion_write)strncat;
	server.root_handler=onion_handler_static("", "Succedded", 200);
	
	onion_request *req=onion_request_new(&server, buffer);
#define S "GET / HTTP/1.1\nHeader-1: This is header1 n Header-2: This is header 2 n Header-1: This is header1 n Header-2: This is header 2 n Header-1: This is header1 n Header-2: This is header 2 n Header-1: This is header1 n Header-2: This is header 2 n Header-1: This is header1 n Header-2: This is header 2 n Header-1: This is header1 n Header-2: This is header 2 n Header-1: This is header1 n Header-2: This is header 2 n Header-1: This is header1 n Header-2: This is header 2 n Header-1: This is header1 n Header-2: This is header 2 n Header-1: This is header1 n Header-2: This is header 2 n Header-1: This is header1 n Header-2: This is header 2 n Header-1: This is header1 n Header-2: This is header 2 n Header-1: This is header1 n Header-2: This is header 2 n Header-1: This is header1 n Header-2: This is header 2 n Header-1: This is header1 n Header-2: This is header 2 n Header-1: This is header1 n Header-2: This is header 2 n Header-1: This is header1 n Header-2: This is header 2 n Header-1: This is header1 n Header-2: This is header 2 n Header-1: This is header1 n Header-2: This is header 2 n Header-1: This is header1 n Header-2: This is header 2 n Header-1: This is header1 n Header-2: This is header 2 n Header-1: This is header1 n Header-2: This is header 2 n Header-1: This is header1 n Header-2: This is header 2 n Header-1: This is header1 n Header-2: This is header 2 n Header-1: This is header1 n Header-2: This is header 2 n Header-1: This is header1 n Header-2: This is header 2 n Header-1: This is header1 n Header-2: This is header 2 n Header-1: This is header1 n Header-2: This is header 2 n Header-1: This is header1 n Header-2: This is header 2 n Header-1: This is header1 n Header-2: This is header 2 n Header-1: This is header1 n Header-2: This is header 2 n Header-1: This is header1 n Header-2: This is header 2\n"
	onion_request_write(req, S,sizeof(S)-1); // send it all, but the final 0.
#undef S
	FAIL_IF_NOT_EQUAL_STR(buffer,"");
	onion_request_write(req, "\n",1); // finish this request. no \n\n before to check possible bugs.
	
	FAIL_IF_EQUAL_STR(buffer,"");
	FAIL_IF_NOT_EQUAL_STR(buffer,"HTTP/1.1 200 OK\nContent-Length: 9\n\nSuccedded");

	onion_request_free(req);
	onion_handler_free(server.root_handler);

	END_LOCAL();
}


int write_p(int *p, const char *data, unsigned length){
	return write(*p, data,length);
}

void t05_server_with_pipes(){
	INIT_LOCAL();
	
	onion_server server;
	server.write=(onion_write)write_p;
	server.root_handler=onion_handler_static("", "Works with pipes", 200);
	
	int p[2];
	pipe(p);
	
	onion_request *req=onion_request_new(&server, &p[1]);
#define S "GET / HTTP/1.1\n\n"
	onion_request_write(req, S,sizeof(S)-1); // send it all, but the final 0.
#undef S

	// read from the pipe
	char buffer[1024];
	memset(buffer,0,sizeof(buffer)); // better clean it, becaus if this doe snot work, it might get an old value
	read(p[0], buffer, sizeof(buffer));
	
	FAIL_IF_EQUAL_STR(buffer,"");
	FAIL_IF_NOT_EQUAL_STR(buffer,"HTTP/1.1 200 OK\nContent-Length: 16\n\nWorks with pipes");

	onion_request_free(req);
	onion_handler_free(server.root_handler);

	END_LOCAL();
}


int main(int argc, char **argv){
	t01_server_min();
	t02_server_full();
	t03_server_no_overflow();
	t04_server_overflow();
	t05_server_with_pipes();
	
	END();
}

