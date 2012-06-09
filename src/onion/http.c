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

#include <malloc.h>
#include <stdlib.h>

#include "types.h"
#include "http.h"
#include "types_internal.h"
#include "connection.h"
#include "listen_point.h"
#include "request.h"
#include "log.h"

static void onion_http_init_connection(onion_connection *op);
static ssize_t onion_http_read(onion_connection *con, char *data, size_t len);
ssize_t onion_http_write(onion_connection *con, const char *data, size_t len);
static void onion_http_close(onion_connection *con);
static int onion_http_read_ready(onion_connection *op);
void onion_http_free_connection_user_data(void *con);

struct onion_http_t{
	onion_request *req;
};

#define BUFFER_SIZE 1500

onion_listen_point* onion_http_new()
{
	onion_listen_point *ret=onion_listen_point_new();
	
	ret->init_connection=onion_http_init_connection;
	ret->close=onion_http_close;
	ret->read=onion_http_read;
	ret->write=onion_http_write;
	
	return ret;
}

static void onion_http_init_connection(onion_connection *con){
	con->user_data=malloc(sizeof(struct onion_http_t)+BUFFER_SIZE);
	struct onion_http_t *ot=(struct onion_http_t*)con->user_data;
	ot->req=onion_request_new(con);
	
	con->free_user_data=onion_http_free_connection_user_data;
	con->read_ready=onion_http_read_ready;
}
static ssize_t onion_http_read(onion_connection *con, char *data, size_t len){
	return read(con->fd, data, len);
}

static int onion_http_read_ready(onion_connection *con){
	char *buffer=con->user_data+sizeof(struct onion_http_t);
	ssize_t len=con->listen_point->read(con, buffer, BUFFER_SIZE);
	
	if (len<0)
		return OCS_CLOSE_CONNECTION;
	
	struct onion_http_t *ot=(struct onion_http_t*)con->user_data;
	onion_connection_status st=onion_request_write(ot->req, buffer, len);
	if (st!=OCS_NEED_MORE_DATA){
		if (st<0)
			return st;
	}
	
	return OCS_PROCESSED;
}

ssize_t onion_http_write(onion_connection *con, const char *data, size_t len){
	return write(con->fd, data, len);
}

void onion_http_free_connection_user_data(void *user_data){
	ONION_DEBUG("Free http user data");
	struct onion_http_t *ot=(struct onion_http_t*)user_data;
	onion_request_free(ot->req);
	free(user_data);
}

static void onion_http_close(onion_connection *con){
	int fd=con->fd;
	shutdown(fd,SHUT_RDWR);
	close(fd);
}
