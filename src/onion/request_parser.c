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
#include <stdlib.h>
#include <libgen.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>

#include "server.h"
#include "dict.h"
#include "request.h"
#include "response.h"
#include "handler.h"
#include "server.h"
#include "types_internal.h"
#include "codecs.h"
#include "log.h"

#define QUERY_BUFFER_SIZE 1020
/**
 * @short Write some data into the request, and passes it line by line to onion_request_fill
 *
 * It features a state machine, from req->parse_state.
 *
 * Depending on the state input is redirected to a diferent parser, one for headers, POST url encoded data... 
 * 
 * @return Returns the number of bytes writen, or <=0 if connection should close, acording to onion_connection_status
 * @see onion_connection_status
 */
onion_connection_status onion_request_write(onion_request *req, const char *data, size_t length){
	return OCS_INTERNAL_ERROR;
}

