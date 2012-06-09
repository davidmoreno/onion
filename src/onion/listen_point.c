/*
	Onion HTTP server library
	Copyright (C) 2012 David Moreno Montero

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 3.0 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not see <http://www.gnu.org/licenses/>.
	*/

#include <string.h>
#include <malloc.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>

#include "types_internal.h"
#include "log.h"

onion_listen_point *onion_listen_point_new(){
	onion_listen_point *ret=calloc(1,sizeof(onion_listen_point));
	return ret;
}

void onion_listen_point_free(onion_listen_point *op){
	if (op->free_user_data)
		op->free_user_data(op->user_data);
	if (op->hostname)
		free(op->hostname);
	if (op->port)
		free(op->port);
	free(op);
}

int onion_listen_point_listen(onion_listen_point *op){
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int sockfd;

	memset(&hints,0, sizeof(struct addrinfo));
	hints.ai_canonname=NULL;
	hints.ai_addr=NULL;
	hints.ai_next=NULL;
	hints.ai_socktype=SOCK_STREAM;
	hints.ai_family=AF_UNSPEC;
	hints.ai_flags=AI_PASSIVE|AI_NUMERICSERV;
	
	ONION_DEBUG("Trying to listen at %s:%s", op->hostname, op->port);

	if (getaddrinfo(op->hostname, op->port, &hints, &result) !=0 ){
		ONION_ERROR("Error getting local address and port: %s", strerror(errno));
		return errno;
	}

	int optval=1;
	for(rp=result;rp!=NULL;rp=rp->ai_next){
		sockfd=socket(rp->ai_family, rp->ai_socktype | SOCK_CLOEXEC, rp->ai_protocol);
		if(SOCK_CLOEXEC == 0) { // Good compiler know how to cut this out
			int flags=fcntl(sockfd, F_GETFD);
			if (flags==-1){
				ONION_ERROR("Retrieving flags from listen socket");
			}
			flags|=FD_CLOEXEC;
			if (fcntl(sockfd, F_SETFD, flags)==-1){
				ONION_ERROR("Setting O_CLOEXEC to listen socket");
			}
		}
		if (sockfd<0) // not valid
			continue;
		if (setsockopt(sockfd,SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval) ) < 0){
			ONION_ERROR("Could not set socket options: %s",strerror(errno));
		}
		if (bind(sockfd, rp->ai_addr, rp->ai_addrlen) == 0)
			break; // Success
		close(sockfd);
	}
	if (rp==NULL){
		ONION_ERROR("Could not find any suitable address to bind to.");
		return errno;
	}

#ifdef __DEBUG__
	char address[64];
	getnameinfo(rp->ai_addr, rp->ai_addrlen, address, 32,
							&address[32], 32, NI_NUMERICHOST | NI_NUMERICSERV);
	ONION_DEBUG("Listening to %s:%s",address,&address[32]);
#endif
	freeaddrinfo(result);
	listen(sockfd,5); // queue of only 5.
	
	op->listenfd=sockfd;
	return 0;
}
