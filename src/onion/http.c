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
#include "parser_block.h"

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
int onion_http_read_ready(onion_request *req){
	char buffer[1500];
	ssize_t len=req->connection.listen_point->read(req, buffer, sizeof(buffer));
	
	if (len<=0)
		return OCS_CLOSE_CONNECTION;
	
	return onion_http_parse(req, buffer, len);
}

/**
 * @short Parses a buffer of data.
 * 
 * It sends the data to the appropiate parser (normally the http, but may be the 
 * websockets or SPDY). 
 * 
 * @param req The request that will receive the data, filling it
 *            or passing the data to the appropiate parser
 * @param buffer Read-Write buffer with the data. The parser is allowed
 *            and encouraged to modify the data there to acomodate to its 
 *            needs.
 * @param len Length of the buffer.
 * 
 * Please be careful as the buffer will be modified, to pass a proper read-write
 * memory area.
 * 
 * @returns How the connectionshould be kept, normally OCS_NEED_MORE_DATA, 
 *          OCS_CLOSE_CONNECTION or OCS_INTERNAL_ERROR. Data can be continued to be
 *          feed if the value is >=0.
 */
onion_connection_status onion_http_parse(onion_request *req, char *buffer, size_t len){
	onion_parser_block rwbuffer;
	onion_parser_block_init(&rwbuffer, buffer, len);
	
	onion_connection_status st=OCS_INTERNAL_ERROR;
	while (req->parser.parse && !onion_parser_block_eof(&rwbuffer)){
		size_t len=onion_parser_block_remaining(&rwbuffer);
		st=req->parser.parse(req, &rwbuffer);
		if (len == onion_parser_block_remaining(&rwbuffer)){
			ONION_ERROR("Parser did not consume data. Bogus parser, aborting petition.");
			return OCS_INTERNAL_ERROR;
		}
		ONION_DEBUG0("%d bytes left", onion_parser_block_remaining(&rwbuffer));
		if (st<0)
			return st;
		if (st==OCS_REQUEST_READY){
			st=onion_request_process(req);
			if (!req->parser.parse){
				ONION_DEBUG("Setting again http parser for this request: %d bytes left", onion_parser_block_remaining(&rwbuffer));
				onion_http_parser_init(req);
			}
		}
	}
	
	return st;
}



/**
 * @short Same as onion_http_parse but allows const buffers
 * 
 * Uses internal buffering to allow to pass const buffers. This is a helper that
 * has some performance impact, so it should be avioided if possible, but
 * some frameworks may need it, as the testing one-
 * 
 */
onion_connection_status onion_http_parse_const(onion_request *req, const char *data, size_t len){
	char *tmp=alloca(len);
	memcpy(tmp, data, len);
	return onion_http_parse(req, tmp, len);
}

/**
 * @short Write data to the HTTP client
 * @memberof onion_http_t
 */
ssize_t onion_http_write(onion_request *con, const char *data, size_t len){
	return write(con->connection.fd, data, len);
}

