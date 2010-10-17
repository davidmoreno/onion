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
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "onion.h"
#include "onion_handler.h"
#include "onion_server.h"
#include "onion_types_internal.h"

/// Internal processor of just one request.
static void onion_process_request(onion *o, int clientfd);

/// Creates the onion structure to fill with the server data, and later do the onion_listen()
onion *onion_new(int flags){
	onion *o=malloc(sizeof(onion));
	o->flags=flags;
	o->listenfd=0;
	o->server=malloc(sizeof(onion_server));
	o->server->root_handler=NULL;
	o->port=8080;
	
	return o;
}

static int write_to_socket(int *fd, const char *data, unsigned int len){
	return write(*fd, data, len);
}

/**
 * @short Performs the listening with the given mode
 *
 * This is the main loop for the onion server.
 *
 * Returns if there is any error. It returns actualy errno from the network operations. See socket for more information.
 */
int onion_listen(onion *o){
	int sockfd;
	sockfd=socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in serv_addr, cli_addr;

	if (sockfd<0)
		return errno;
	memset((char *) &serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(o->port);
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		return errno;
	listen(sockfd,1);
  socklen_t clilen = sizeof(cli_addr);

	if (o->flags&O_ONE){
		int clientfd;
		if (o->flags&O_ONE_LOOP){
			while(1){
				clientfd=accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
				onion_process_request(o, clientfd);
				close(clientfd);
			}
		}
		else{
			clientfd=accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
			onion_process_request(o, clientfd);
			close(clientfd);
		}
	}
	close(sockfd);
	return 0;
}

/// Removes the allocated data
void onion_free(onion *onion){
	onion_server_free(onion->server);
	free(onion);
}

/// Sets the root handler
void onion_set_root_handler(onion *onion, onion_handler *handler){
	onion->server->root_handler=handler;
}

void onion_set_port(onion *server, int port){
	server->port=port;
}

/**
 * @short Internal processor of just one request.
 *
 * It can be used on one processing, on threaded, on one_loop...
 */
static void onion_process_request(onion *o, int clientfd){
	int r,w;
	char buffer[1024];
	onion_request *req=onion_request_new(o->server, &clientfd);
	onion_server_set_write(o->server, (onion_write)write_to_socket);
	while ( ( r=read(clientfd, buffer, sizeof(buffer)) ) >0){
		//fprintf(stderr, "%s:%d Read %d bytes\n",__FILE__,__LINE__,r);
		w=onion_request_write(req, buffer, r);
		if (w<0){ // request processed.
			return;
		}
	}
}
