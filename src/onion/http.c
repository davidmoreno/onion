/*
	Onion HTTP server library
	Copyright (C) 2010-2014 David Moreno Montero and othes

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

#include <stdlib.h>

#include "types.h"
#include "http.h"
#include "types_internal.h"
#include "listen_point.h"
#include "request.h"
#include "log.h"

static ssize_t onion_http_read(onion_request *req, char *data, size_t len);
ssize_t onion_http_write(onion_request *req, const char *data, size_t len);
int onion_http_read_ready(onion_request *req);

/**
 * @struct onion_http_t
 * @memberof onion_http_t
 */
struct onion_http_t{
};

/**
 * @short Creates an HTTP listen point 
 * @memberof onion_http_t
 */
onion_listen_point* onion_http_new()
{
	onion_listen_point *ret=onion_listen_point_new();
	
	ret->read=onion_http_read;
	ret->write=onion_http_write;
	ret->close=onion_listen_point_request_close_socket;
	ret->read_ready=onion_http_read_ready;
	
	return ret;
}

/**
 * @short Reads data from the http connection
 * @memberof onion_http_t
 */
static ssize_t onion_http_read(onion_request *con, char *data, size_t len){
	return read(con->connection.fd, data, len);
}

/**
 * @short HTTP client has data ready to be readen
 * @memberof onion_http_t
 */
int onion_http_read_ready(onion_request *con){
	char buffer[1500];
	ssize_t len=con->connection.listen_point->read(con, buffer, sizeof(buffer));
	
	if (len<=0)
		return OCS_CLOSE_CONNECTION;
	
	onion_connection_status st=onion_request_write(con, buffer, len);
	if (st!=OCS_NEED_MORE_DATA){
		if (st<0)
			return st;
	}
	
	return OCS_PROCESSED;
}

/**
 * @short Write dat to the HTTP client
 * @memberof onion_http_t
 */
ssize_t onion_http_write(onion_request *con, const char *data, size_t len){
	return write(con->connection.fd, data, len);
}

