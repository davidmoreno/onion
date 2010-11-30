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

#ifndef __ONION_REQUEST__
#define __ONION_REQUEST__

#include "onion_types.h"

#ifdef __cplusplus
extern "C"{
#endif

/**
 * @short Methods in which data can be asked.
 */
enum onion_request_flags_e{
	OR_GET=1,
	OR_POST=2,
	OR_HEAD=4,
	
	OR_HTTP11=0x10,
	
	OR_NO_KEEP_ALIVE=0x100,
};

typedef enum onion_request_flags_e onion_request_flags;


/// Creates a request
onion_request *onion_request_new(onion_server *server, void *socket, const char *client_info);

/// Deletes a request and all its data
void onion_request_free(onion_request *req);

/// Partially fills a request. One line each time.
int onion_request_fill(onion_request *req, const char *data);

/// Reads some data from the input (net, file...) and performs the onion_request_fill
onion_connection_status onion_request_write(onion_request *req, const char *data, size_t length);

/// Gets the current path
const char *onion_request_get_path(onion_request *req);

/// Gets the current flags, as in onion_request_flags_e
onion_request_flags onion_request_get_flags(onion_request *req);

/// Moves the path pointer to later in the fullpath
void onion_request_advance_path(onion_request *req, int addtopos);

/// Gets a header data
const char *onion_request_get_header(onion_request *req, const char *header);

/// Gets query data
const char *onion_request_get_query(onion_request *req, const char *query);

/// Gets post data
const char *onion_request_get_post(onion_request *req, const char *query);

/// Gets the header header data dict
const onion_dict *onion_request_get_header_dict(onion_request *req);

/// Gets request query dict
const onion_dict *onion_request_get_query_dict(onion_request *req);

/// Gets post data dict
const onion_dict *onion_request_get_post_dict(onion_request *req);

/// Cleans the request object, to reuse it
void onion_request_clean(onion_request *req);

/// Reqeust to close connection after one request is done, forces no keep alive.
void onion_request_set_no_keep_alive(onion_request *req);

/// Returns if current request wants to keep alive
int onion_request_keep_alive(onion_request *req);

#ifdef __cplusplus
}
#endif

#endif
