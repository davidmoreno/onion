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
int onion_http_read_ready(onion_connection *op);
static void onion_http_close(onion_connection *con);

void onion_request_deinit(onion_request *req);
void onion_request_init(onion_request *req, onion_connection *con);


// If modified, modify too at https
struct onion_http_t{
	onion_request req;
};

onion_listen_point* onion_http_new()
{
	onion_listen_point *ret=onion_listen_point_new();
	
	ret->connection_init=onion_http_init_connection;
	ret->read=onion_http_read;
	ret->write=onion_http_write;
	ret->close=onion_http_close;
	ret->read_ready=onion_http_read_ready;
	
	return ret;
}

static void onion_http_init_connection(onion_connection *con){
	con->user_data=malloc(sizeof(struct onion_http_t));
	struct onion_http_t *ot=(struct onion_http_t*)con->user_data;
	onion_request_init(&ot->req, con);
}

static ssize_t onion_http_read(onion_connection *con, char *data, size_t len){
	return read(con->fd, data, len);
}

int onion_http_read_ready(onion_connection *con){
	char buffer[1500];
	ssize_t len=con->listen_point->read(con, buffer, sizeof(buffer));
	
	if (len<0)
		return OCS_CLOSE_CONNECTION;
	
	struct onion_http_t *ot=(struct onion_http_t*)con->user_data;
	onion_connection_status st=onion_request_write(&ot->req, buffer, len);
	if (st!=OCS_NEED_MORE_DATA){
		if (st<0)
			return st;
	}
	
	return OCS_PROCESSED;
}

ssize_t onion_http_write(onion_connection *con, const char *data, size_t len){
	return write(con->fd, data, len);
}

static void onion_http_close(onion_connection *con){
	onion_connection_close_socket(con);
	struct onion_http_t *ot=(struct onion_http_t*)con->user_data;
	ONION_DEBUG("Free http user data");
	onion_request_deinit(&ot->req);
	free(con->user_data);
}

