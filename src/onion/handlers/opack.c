/*
	Onion HTTP server library
	Copyright (C) 2010-2013 David Moreno Montero

	This library is free software; you can redistribute it and/or
	modify it under the terms of, at your choice:
	
	a. the GNU Lesser General Public License as published by the 
	 Free Software Foundation; either version 3.0 of the License, 
	 or (at your option) any later version.
	
	b. the GNU General Public License as published by the 
	 Free Software Foundation; either version 2.0 of the License, 
	 or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License and the GNU General Public License along with this 
	library; if not see <http://www.gnu.org/licenses/>.
	*/

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <regex.h>

#include <onion/handler.h>
#include <onion/response.h>

#include "opack.h"

struct onion_handler_opack_data_t{
	unsigned int length;
	char *path;
	onion_opack_renderer render;
};

typedef struct onion_handler_opack_data_t onion_handler_opack_data;

int onion_handler_opack_handler(onion_handler_opack_data *d, onion_request *request, onion_response *res){
	if (strcmp(d->path, onion_request_get_path(request))!=0)
		return 0;
		
	if (d->length)
		onion_response_set_length(res, d->length);
	onion_response_write_headers(res);

	d->render(res);
	
	return OCS_PROCESSED;
}


void onion_handler_opack_delete(onion_handler_opack_data *data){
	free(data->path);
	free(data);
}

/**
 * @short Creates an opack handler that calls the onion_opack_renderer with length data.
 *
 * If on the inside level nobody answers, it just returns NULL, so ->next can answer.
 * 
 * @param path Path of the current data, for example /. It is a normal string; no regular expressions are allowed.
 * @param render Function to call to render the response.
 * @param length Lenght of the data, or 0 if unknown. Needed to keep alive.
 */
onion_handler *onion_handler_opack(const char *path, onion_opack_renderer render, unsigned int length){
	onion_handler_opack_data *priv_data=malloc(sizeof(onion_handler_opack_data));

	priv_data->path=strdup(path);
	priv_data->length=length;
	priv_data->render=render;
	
	onion_handler *ret=onion_handler_new((onion_handler_handler)onion_handler_opack_handler,
																			 priv_data, (onion_handler_private_data_free) onion_handler_opack_delete);
	return ret;
}

