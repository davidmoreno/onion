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

#include "onion_dict.h"
#include "onion_response.h"
#include "onion_server.h"
#include "onion_handler.h"
#include "onion_types_internal.h"
#include "onion_log.h"

/// Default error 500.
int error_500(void *handler, onion_request *req);

/**
 * @short Initializes the server data.
 * 
 * This data is independant of communication channel, and its the data that should be alsays available, even shared between the channels.
 * 
 * All data have sensitive defaults, except write and root_handler, and can be modified with accessor functions.
 * 
 * These defaults include: 
 *  * internal_error_handler
 *  * max_post_size -- 1MB
 *  * max_file_size -- 1GB
 */
onion_server *onion_server_new(void){
	onion_server *ret=malloc(sizeof(onion_server));
	ret->root_handler=NULL;
	ret->write=NULL;
	ret->internal_error_handler=onion_handler_new((onion_handler_handler)error_500, NULL, NULL);
	ret->max_post_size=1024*1024; // 1MB
	ret->max_file_size=1024*1024*1024; // 1GB
	return ret;
}

/**
 * @short Cleans all data used by this server
 */
void onion_server_free(onion_server *server){
	if (server->root_handler)
		onion_handler_free(server->root_handler);
	if (server->internal_error_handler)
		onion_handler_free(server->internal_error_handler);
	free(server);
}

/**
 * @short Sets the writer function. 
 * 
 * It has the signature ssize_t (*onion_write)(void *handler, const char *data, unsigned int length). the handler is passed to the request.
 */
void onion_server_set_write(onion_server *server, onion_write write){
	server->write=write;
}

/**
 * @short Sets the root handler that handles all the requests.
 */
void onion_server_set_root_handler(onion_server *server, onion_handler *handler){
	server->root_handler=handler;
}

/**
 * @short Sets the error handler that will be called to return data to users on internal errors (5XX).
 */
void onion_server_set_internal_error_handler(onion_server *server, onion_handler *handler){
	if (server->internal_error_handler)
		onion_handler_free(server->internal_error_handler);
	server->internal_error_handler=handler;
}

/**
 * @short Sets the maximum post size
 * 
 * The post data can be large, but it should not be allowed to be infinite. This is the maximum allowed size.
 * This size is not net size, as it may come encoded and that takes extra space (for example base64 encodings
 * take aprox 1/4 of extra space + variable names, control chars).
 */
void onion_server_set_max_post_size(onion_server *server, size_t max_post_size){
	server->max_post_size=max_post_size;
}

/**
 * @short  Sets the maximum file size
 * 
 * Sets the maximum allowed file size in total, adding the size of all sent files.
 * 
 * This is real size on disk. On the request object the files dicitonary will be filed with the
 * field names pointing to disk temporal files. The temporal files will have the real basename, but
 * will be stored ina temporary position, and deleted at request delete.
 * 
 * @see onion_request_get_file
 */
void onion_server_set_max_file_size(onion_server *server, size_t max_file_size){
	server->max_file_size=max_file_size;
}

/**
 * @short  Performs the processing of the request.
 * 
 * Returns the OCS_KEEP_ALIVE or OCS_CLOSE_CONNECTION value. If on keep alive the struct is already reinitialized.
 * 
 * @see onion_connection_status
 */
int onion_server_handle_request(onion_request *req){
	ONION_DEBUG("Handling request");
	int status=onion_handler_handle(req->server->root_handler, req);
	if (status==OCS_KEEP_ALIVE) // if keep alive, reset struct to get the new petition.
		onion_request_clean(req);
	return status;
}


#define ERROR_500 "<h1>500 - Internal error</h1> Check server logs or contact administrator."

/**
 * @short Defautl error for 500 Internal error.
 * 
 * Not the right way to do it.. FIXME.
 */
int error_500(void *handler, onion_request *req){
	onion_response *res=onion_response_new(req);
	onion_response_set_code(res,500);
	onion_response_set_length(res, sizeof(ERROR_500)-1);
	onion_response_write_headers(res);
	
	onion_response_write0(res,ERROR_500);
	return onion_response_free(res);
}

