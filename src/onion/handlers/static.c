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

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <regex.h>

#include <onion/handler.h>
#include <onion/response.h>
#include <onion/low.h>

#include "static.h"

struct onion_handler_static_data_t{
	int code;
	const char *data;
};

typedef struct onion_handler_static_data_t onion_handler_static_data;

/**
 * @short Performs the real request: set the code and write data
 */
int onion_handler_static_handler(onion_handler_static_data *d, onion_request *request, onion_response *res){
	int length=strlen(d->data);
	onion_response_set_length(res, length);
	onion_response_set_code(res, d->code);
	
	onion_response_write_headers(res);
	//fprintf(stderr,"Write %d bytes\n",length);
	onion_response_write(res, d->data, length);

	return OCS_PROCESSED;
}

/// Removes internal data for this handler.
void onion_handler_static_delete(onion_handler_static_data *d){
	onion_low_free((char*)d->data);
	onion_low_free(d);
}

/**
 * @short Creates a static handler that just writes some static data.
 *
 * Path is a regex for the url, as arrived here.
 */
onion_handler *onion_handler_static(const char *text, int code){
	onion_handler_static_data *priv_data=onion_low_malloc(sizeof(onion_handler_static_data));
	if (!priv_data)
		return NULL;

	priv_data->code=code;
	priv_data->data=onion_low_strdup(text);

	onion_handler *ret=onion_handler_new((onion_handler_handler)onion_handler_static_handler,
																			 priv_data,(onion_handler_private_data_free) onion_handler_static_delete);
	return ret;
}

