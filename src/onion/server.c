/*
	Onion HTTP server library
	Copyright (C) 2010 David Moreno Montero

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

#include "dict.h"
#include "response.h"
#include "server.h"
#include "handler.h"
#include "types_internal.h"
#include "log.h"
#include "sessions.h"
#include "mime.h"

/// Default error handler.
static int onion_default_error(void *handler, onion_request *req, onion_response *res);

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
	ret->internal_error_handler=onion_handler_new((onion_handler_handler)onion_default_error, NULL, NULL);
	ret->max_post_size=1024*1024; // 1MB
	ret->max_file_size=1024*1024*1024; // 1GB
	ret->sessions=onion_sessions_new();
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
	onion_mime_set(NULL);
	onion_sessions_free(server->sessions);
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
 * @short Sets the error handler that will be called to return data to users on errors.
 * 
 * On the called handler, the request objet will have at flags the reason of 
 * the error: OR_NOT_FOUND, OR_NOT_IMPLEMENTED or OR_INTERNAL_ERROR
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
 * It calls the root_handler and if it returns an error of any type (or not processed), it calls the
 * default internal_error_handler.
 * 
 * Normally it could get the server from the request, but it is passed for homogenity, and
 * to allow unforseen posibilities.
 * 
 * @see onion_connection_status
 */
onion_connection_status onion_server_handle_request(onion_server *server, onion_request *req){
	onion_response *res=onion_response_new(req);
	
	// Call the main handler.
	onion_connection_status hs=onion_handler_handle(server->root_handler, req, res);

	if (hs==OCS_INTERNAL_ERROR || 
		hs==OCS_NOT_IMPLEMENTED || 
		hs==OCS_NOT_PROCESSED){
		if (hs==OCS_INTERNAL_ERROR)
			req->flags|=OR_INTERNAL_ERROR;
		if (hs==OCS_NOT_IMPLEMENTED)
			req->flags|=OR_NOT_IMPLEMENTED;
		if (hs==OCS_NOT_PROCESSED)
			req->flags|=OR_NOT_FOUND;
		
		hs=onion_handler_handle(server->internal_error_handler, req, res);
	}

	
	int rs=onion_response_free(res);
	if (hs>=0 && rs==OCS_KEEP_ALIVE) // if keep alive, reset struct to get the new petition.
		onion_request_clean(req);
	
	return hs>0 ? rs : hs;
}


#define ERROR_500 "<h1>500 - Internal error</h1> Check server logs or contact administrator."
#define ERROR_404 "<h1>404 - Not found</h1>"
#define ERROR_505 "<h1>505 - Not implemented</h1>"

/**
 * @short Default error printer. 
 * 
 * Ugly errors, that can be reimplemented setting a handler with onion_server_set_internal_error_handler.
 */
static int onion_default_error(void *handler, onion_request *req, onion_response *res){
	const char *msg;
	int l;
	int code;
	switch(req->flags&0x0F000){
		case OR_INTERNAL_ERROR:
			msg=ERROR_500;
			l=sizeof(ERROR_500)-1;
			code=HTTP_INTERNAL_ERROR;
			break;
		case OR_NOT_IMPLEMENTED:
			msg=ERROR_505;
			l=sizeof(ERROR_505)-1;
			code=HTTP_NOT_IMPLEMENTED;
			break;
		default:
			msg=ERROR_404;
			l=sizeof(ERROR_404)-1;
			code=HTTP_NOT_FOUND;
			break;
	}
	
	ONION_DEBUG0("Internally managed error: %s, code %d.", msg, code);
	
	onion_response_set_code(res,code);
	onion_response_set_length(res, l);
	onion_response_write_headers(res);
	
	onion_response_write(res,msg,l);
	return OCS_PROCESSED;
}

/**
 * @short Tells the server to write something on that request.
 * 
 * It takes care of moments when it has to process the request, and maybe there was an error, so
 * it calls the error handlers.
 */
onion_connection_status onion_server_write_to_request(onion_server *server, onion_request *request, const char *data, size_t len){
	// This is one maybe unnecesary indirection, but helps when creating new "onion" like classes.
	onion_connection_status connection_status=onion_request_write(request, data, len);
	return connection_status;
}
