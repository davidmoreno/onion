/*
	Onion HTTP server library
	Copyright (C) 2010-2016 David Moreno Montero and others

	This library is free software; you can redistribute it and/or
	modify it under the terms of, at your choice:

	a. the Apache License Version 2.0.

	b. the GNU General Public License as published by the
		Free Software Foundation; either version 2.0 of the License,
		or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of both libraries, if not see
	<http://www.gnu.org/licenses/> and
	<http://www.apache.org/licenses/LICENSE-2.0>.
	*/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE             /* See feature_test_macros(7) */
#endif
#include <sys/socket.h>

#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>

#include "types_internal.h"
#include "low.h"
#include "log.h"
#include "poller.h"
#include "request.h"
#include "listen_point.h"

#ifndef SOCK_CLOEXEC
#define SOCK_CLOEXEC 0
#define accept4(a,b,c,d) accept(a,b,c);
#endif

#ifdef HAVE_SYSTEMD
#include <systemd/sd-daemon.h>
#endif

/// @defgroup listen_point Listen Point. Allows to listen at several ports with different protocols, and to add new protocols.

static int onion_listen_point_read_ready(onion_request *req);


/**
 * @short Creates an empty listen point.
 * @memberof onion_listen_point_t
 * @ingroup listen_point
 *
 * Called by real listen points to ease the creation.
 *
 * @returns An alloc'ed onion_listen_point pointer
 */
onion_listen_point *onion_listen_point_new(){
	onion_listen_point *ret=onion_low_calloc(1,sizeof(onion_listen_point));
	return ret;
}

/**
 * @short Free and closes the listen point
 * @memberof onion_listen_point_t
 * @ingroup listen_point
 *
 * Calls the custom listen_stop mathod, and frees all common structures.
 *
 * @param op the listen point
 */
void onion_listen_point_free(onion_listen_point *op){
	ONION_DEBUG("Free listen point %d", op->listenfd);
	onion_listen_point_listen_stop(op);
	if (op->free_user_data)
		op->free_user_data(op);
	if (op->hostname)
		onion_low_free(op->hostname);
	if (op->port)
		onion_low_free(op->port);
	onion_low_free(op);
}


/**
 * @short Called when a new connection appears on the listenfd
 * @memberof onion_listen_point_t
 * @ingroup listen_point
 *
 * When the new connection appears, creates the request and adds it to the pollers.
 *
 * It returns always 1 as any <0 would detach from the poller and close the listen point,
 * and not accepting a request does not mean the connection point is corrupted. If a
 * connection point may become corrupted should be the connection point itself who detaches
 * from the poller.
 *
 * @param op The listen point from where the request must be built
 * @returns 1 always. The poller needs one to keep listening for connections.
 */
int onion_listen_point_accept(onion_listen_point *op){
	onion_request *req=onion_request_new(op);
	if (req){
		if (req->connection.fd>0){
			onion_poller_slot *slot=onion_poller_slot_new(req->connection.fd, (void*)onion_listen_point_read_ready, req);
			if (!slot)
				return 1;
			onion_poller_slot_set_timeout(slot, req->connection.listen_point->server->timeout);
			onion_poller_slot_set_shutdown(slot, (void*)onion_request_free, req);
			onion_poller_add(req->connection.listen_point->server->poller, slot);
			return 1;
		}
		// No fd. This could mean error, or not fd based. Normally error would not return a req.
		onion_request_free(req);
		ONION_ERROR("Error creating connection");
		return 1;
	}

	return 1;
}

/**
 * @short Stops listening the listen point
 * @memberof onion_listen_point_t
 * @ingroup listen_point
 *
 * Calls the op->listen_stop if any, and if not just closes the listenfd.
 *
 * @param op The listen point
 */
void onion_listen_point_listen_stop(onion_listen_point *op){
	if (op->listen_stop)
		op->listen_stop(op);
	else{
		if (op->listenfd>=0){
			shutdown(op->listenfd,SHUT_RDWR);
			close(op->listenfd);
			op->listenfd=-1;
		}
	}
}

/**
 * @short Starts the listening phase for this listen point for sockets.
 * @memberof onion_listen_point_t
 * @ingroup listen_point
 *
 * Default listen implementation that listens on sockets. Opens sockets and setup everything properly.
 *
 * @param op The listen point
 * @returns 0 if ok, !=0 some error; it will be the errno value.
 */
int onion_listen_point_listen(onion_listen_point *op){
	if (op->listen){
			op->listen(op);
			return 0;
	}
#ifdef HAVE_SYSTEMD
	if (op->server->flags&O_SYSTEMD){
		int n=sd_listen_fds(0);
		ONION_DEBUG("Checking if have systemd sockets: %d",n);
		if (n>0){ // If 0, normal startup. Else use the first LISTEN_FDS.
			ONION_DEBUG("Using systemd sockets");
			if (n>1){
				ONION_WARNING("Get more than one systemd socket descriptor. Using only the first.");
			}
			op->listenfd=SD_LISTEN_FDS_START+0;
			return 0;
		}
	}
#endif


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

	ONION_DEBUG("Trying to listen at %s:%s", op->hostname, op->port ? op->port : "8080");

	if (getaddrinfo(op->hostname, op->port ? op->port : "8080", &hints, &result) !=0 ){
		ONION_ERROR("Error getting local address and port: %s", strerror(errno));
		return errno;
	}

	int optval=1;
	for(rp=result;rp!=NULL;rp=rp->ai_next){
		sockfd=socket(rp->ai_family, rp->ai_socktype | SOCK_CLOEXEC, rp->ai_protocol);
		if (sockfd<0) // not valid
			continue;
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
		if (setsockopt(sockfd,SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval) ) < 0){
			ONION_ERROR("Could not set socket options: %s",strerror(errno));
		}
		if (bind(sockfd, rp->ai_addr, rp->ai_addrlen) == 0)
			break; // Success
		else {
			ONION_ERROR("Could not bind to socket: %s",strerror(errno));
		}
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
	ONION_DEBUG("Listening to %s:%s, fd %d",address,&address[32],sockfd);
#endif
	freeaddrinfo(result);
	listen(sockfd,5); // queue of only 5.

	op->listenfd=sockfd;
	return 0;
}

/**
 * @short This listen point has data ready to read; calls the listen_point read_ready
 * @memberof onion_listen_point_t
 * @ingroup listen_point
 *
 * @param req The request with data ready
 * @returns <0 in case of error and request connection should be closed.
 */
static int onion_listen_point_read_ready(onion_request *req){
#ifdef __DEBUG__
	if (!req->connection.listen_point->read_ready){
		ONION_ERROR("read_ready handler not set!");
		return OCS_INTERNAL_ERROR;
	}
#endif

	return req->connection.listen_point->read_ready(req);
}


/**
 * @short Default implementation that initializes the request from a socket
 * @memberof onion_listen_point_t
 * @ingroup listen_point
 *
 * Accepts the connection and initializes it.
 *
 * @param req Request to initialize
 * @returns <0 if error opening the connection
 */
int onion_listen_point_request_init_from_socket(onion_request *req){
	onion_listen_point *op=req->connection.listen_point;
	int listenfd=op->listenfd;
	if (listenfd<0){
		ONION_DEBUG("Listen point closed, no request allowed");
		return -1;
	}

	/// Follows default socket implementation. If your protocol is socket based, just use it.

	req->connection.cli_len = sizeof(req->connection.cli_addr);

	int set_cloexec=SOCK_CLOEXEC == 0;
	int clientfd=accept4(listenfd, (struct sockaddr *) &req->connection.cli_addr,
				&req->connection.cli_len, SOCK_CLOEXEC);
	if (clientfd<0){
		ONION_DEBUG("Second try? errno %d, clientfd %d", errno, clientfd);
		if (errno==ENOSYS){
			clientfd=accept(listenfd, (struct sockaddr *) &req->connection.cli_addr,
					&req->connection.cli_len);
		}
		ONION_DEBUG("How was it? errno %d, clientfd %d", errno, clientfd);
		if (clientfd<0){
			ONION_ERROR("Error accepting connection: %s",strerror(errno),errno);
			onion_listen_point_request_close_socket(req);
			return -1;
		}
	}
	req->connection.fd=clientfd;

	/// Thanks to Andrew Victor for pointing that without this client may block HTTPS connection. It could lead to DoS if occupies all connections.
	{
		struct timeval t;
		t.tv_sec = op->server->timeout / 1000;
		t.tv_usec = ( op->server->timeout % 1000 ) * 1000;

		setsockopt(clientfd, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(struct timeval));
	}

	if(set_cloexec) { // Good compiler know how to cut this out
		int flags=fcntl(clientfd, F_GETFD);
		if (flags==-1){
			ONION_ERROR("Retrieving flags from connection");
		}
		flags|=FD_CLOEXEC;
		if (fcntl(clientfd, F_SETFD, flags)==-1){
			ONION_ERROR("Setting FD_CLOEXEC to connection");
		}
	}

	ONION_DEBUG0("New connection, socket %d",clientfd);
	return 0;
}

/**
 * @short Default implementation that just closes the connection
 * @memberof onion_listen_point_t
 * @ingroup listen_point
 *
 * @param oc The request
 */
void onion_listen_point_request_close_socket(onion_request *oc){
	int fd=oc->connection.fd;
	ONION_DEBUG0("Closing connection socket fd %d",fd);
	if (fd>=0){
		shutdown(fd,SHUT_RDWR);
		close(fd);
		oc->connection.fd=-1;
	}
}
