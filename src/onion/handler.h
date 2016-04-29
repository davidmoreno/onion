/*
	Onion HTTP server library
	Copyright (C) 2010-2016 David Moreno Montero and others

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

#ifndef ONION_HANDLER_H
#define ONION_HANDLER_H

#ifdef __cplusplus
extern "C"{
#endif

#include "request.h"
#include "types.h"

/// checks that handler to handle the request
onion_connection_status onion_handler_handle(onion_handler *handler, onion_request *request, onion_response *response);

/// Creates an onion handler with that private datas.
onion_handler *onion_handler_new(onion_handler_handler handler, void *privdata, onion_handler_private_data_free priv_data_free);

/// Frees the memory of the handler
int onion_handler_free(onion_handler *handler);

/// Adds a handler to the list of handler of this level
void onion_handler_add(onion_handler *base, onion_handler *new_handler);

/// Returns the private data part of the handler. Useful at handlers, to customize the private data externally.
void *onion_handler_get_private_data(onion_handler *handler);

#ifdef __cplusplus
}
#endif

#endif
