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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <onion/request.h>
#include <onion/response.h>
#include <onion/handler.h>
#include <onion/log.h>
#include <onion/block.h>
#include <onion/onion.h>

#include "../ctest.h"
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>

#ifndef SOCK_CLOEXEC
#define SOCK_CLOEXEC 0
#endif

onion_connection_status process_request(void *_, onion_request *req, onion_response *res){
  onion_response_write0(res, "Done");
  
	const onion_block *data=onion_request_get_data(req);
	FAIL_IF_NOT(data);
	FAIL_IF_NOT_EQUAL_STR(onion_block_data(data), "{\n   \"a\": \"10\",\n   \"b\": \"20\"\n}");
	
	ONION_DEBUG(onion_block_data(data));
	
  return OCS_PROCESSED;
}

onion_block *connect_and_send(const char *ip, const char *port, const char *msg){
	int fd;
	{
		struct addrinfo hints;
		struct addrinfo *server;
		
		memset(&hints,0, sizeof(struct addrinfo));
		hints.ai_socktype=SOCK_STREAM;
		hints.ai_family=AF_UNSPEC;
		hints.ai_flags=AI_PASSIVE|AI_NUMERICSERV;

		if (getaddrinfo(ip,port,&hints,&server)<0){
			ONION_ERROR("Error getting server info");
			return NULL;
		}
		fd=socket(server->ai_family, server->ai_socktype | SOCK_CLOEXEC, server->ai_protocol);
		
		if (connect(fd, server->ai_addr, server->ai_addrlen)==-1){
			close(fd);
			fd=-1;
			ONION_ERROR("Error connecting to server %s:%s",ip,port);
			return NULL;
		}
		
		freeaddrinfo(server);
	}

	int ret=write(fd, msg, strlen(msg));
	FAIL_IF(ret<0);
	onion_block *bl=onion_block_new();
	char tmp[256];
	int r=0;
	do{
		r=read(fd, tmp, sizeof(tmp));
		if (r>0)
			onion_block_add_data(bl, tmp, r);
	}while(r>0);
	
	close(fd);
	
	return bl;
}

void POST_json(void){
	sleep(1);
	onion_block *bl=connect_and_send("127.0.0.1","8080", 
		"POST /configuration HTTP/1.1\nHost: example.com\nContent-Type: application/json\nContent-Length: 30\n\n"
		"{\n   \"a\": \"10\",\n   \"b\": \"20\"\n}");

	FAIL_IF_NOT(strstr(onion_block_data(bl),"Done"));
	onion_block_free(bl);
}

void t01_post_no_type(){
	INIT_LOCAL();

	onion *o=onion_new(O_ONE);
	onion_set_root_handler(o, onion_handler_new((void*)process_request, NULL, NULL));
	
	pthread_t pt;
	pthread_create(&pt, NULL, (void*)POST_json, NULL);
	onion_listen(o);
	
	pthread_join(pt, NULL);
	
	onion_free(o);
	
	END_LOCAL();
}

int main(int argc, char **argv){
	START();
	
	t01_post_no_type();
	
	END();
}
