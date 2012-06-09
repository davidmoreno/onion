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

// To have accept4, if possible.
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <malloc.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "types_internal.h"
#include "log.h"
#include "poller.h"
#include "connection.h"

#ifndef SOCK_CLOEXEC
#define SOCK_CLOEXEC 0
#define accept4(a,b,c,d) accept(a,b,c);
#endif

void onion_connection_free(onion_connection *oc){
	if (oc->free_user_data)
		oc->free_user_data(oc->user_data);
	if (oc->cli_info)
		free(oc->cli_info);
	free(oc);
}

static int onion_connection_read_ready(onion_connection *con){
	if (con->read_ready)
		return con->read_ready(con);
	return OCS_INTERNAL_ERROR;
}


int onion_connection_accept(onion_listen_point *p){
	onion_connection *con=onion_connection_new(p);
	if (con){
		onion_poller_slot *slot=onion_poller_slot_new(con->fd, (void*)onion_connection_read_ready, con);
		onion_poller_slot_set_timeout(slot, p->server->timeout);
		onion_poller_slot_set_shutdown(slot, (void*)onion_connection_free, con);
		onion_poller_add(p->server->poller, slot);
		return 1;
	}
	ONION_ERROR("Error creating connection");
	return -1;
}

onion_connection *onion_connection_new(onion_listen_point* op){
	int listenfd=op->listenfd;
	onion_connection *oc=NULL;
	if (op->accept_connection){
		oc=op->accept_connection(op, listenfd);
	}
	else{
		oc=calloc(1,sizeof(onion_connection));
		
		/// Follows default socket implementation. If your protocol is socket based, just use it.
		
		oc->cli_len = sizeof(oc->cli_addr);

		int clientfd=accept4(listenfd, (struct sockaddr *) &oc->cli_addr, &oc->cli_len, SOCK_CLOEXEC);
		if (clientfd<0){
			ONION_ERROR("Error accepting connection: %s",strerror(errno));
			return NULL;
		}
		
		/// Thanks to Andrew Victor for pointing that without this client may block HTTPS connection. It could lead to DoS if occupies all connections.
		{
			struct timeval t;
			t.tv_sec = op->server->timeout / 1000;
			t.tv_usec = ( op->server->timeout % 1000 ) * 1000;

			setsockopt(clientfd, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(struct timeval));
		}
		
		if(SOCK_CLOEXEC == 0) { // Good compiler know how to cut this out
			int flags=fcntl(clientfd, F_GETFD);
			if (flags==-1){
				ONION_ERROR("Retrieving flags from connection");
			}
			flags|=FD_CLOEXEC;
			if (fcntl(clientfd, F_SETFD, flags)==-1){
				ONION_ERROR("Setting FD_CLOEXEC to connection");
			}
		}
		oc->fd=clientfd;
		oc->listen_point=op;
	}
	if (oc){
		if (op->init_connection)
			op->init_connection(oc);
		
		ONION_DEBUG("Accepted connection");
	}
	else{
		ONION_ERROR("Could not create connection!");
	}
	
	return oc;
}
