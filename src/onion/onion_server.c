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


onion_server *onion_server_new(void){
	onion_server *ret=malloc(sizeof(onion_server));
	ret->root_handler=NULL;
	ret->write=NULL;
	ret->internal_error_handler=onion_handler_new((onion_handler_handler)error_500, NULL, NULL);
	return ret;
}

void onion_server_free(onion_server *server){
	if (server->root_handler)
		onion_handler_free(server->root_handler);
	if (server->internal_error_handler)
		onion_handler_free(server->internal_error_handler);
	free(server);
}

void onion_server_set_write(onion_server *server, onion_write write){
	server->write=write;
}

void onion_server_set_root_handler(onion_server *server, onion_handler *handler){
	server->root_handler=handler;
}

void onion_server_set_internal_error_handler(onion_server *server, onion_handler *handler){
	if (server->internal_error_handler)
		onion_handler_free(server->internal_error_handler);
	server->internal_error_handler=handler;
}

/**
 * @short  Performs the processing of the request. FIXME. Make really close the connection on Close-Connection. It does not do it now.
 * 
 * Returns the OR_KEEP_ALIVE or OR_CLOSE_CONNECTION value. If on keep alive the struct is already reinitialized.
 */
int onion_server_handle_request(onion_request *req){
	ONION_DEBUG("Handling request");
	int status=onion_handler_handle(req->server->root_handler, req);
	if (status==OR_KEEP_ALIVE){ // if keep alive, reset struct to get the new petition.
		onion_request_clean(req);
		return OR_KEEP_ALIVE;
	}
	return OR_CLOSE_CONNECTION;
}


#define ERROR_500 "<h1>500 - Internal error</h1> Check server logs or contact administrator."

/**
 * @short Defautl error 500.
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

