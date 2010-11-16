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

#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <regex.h>

#include <onion/onion_handler.h>
#include <onion/onion_response.h>

#include "onion_handler_opack.h"

struct onion_handler_opack_data_t{
	char *path;
	onion_opack_renderer render;
};

typedef struct onion_handler_opack_data_t onion_handler_opack_data;

int onion_handler_opack_handler(onion_handler_opack_data *d, onion_request *request){
	if (strcmp(d->path, onion_request_get_path(request))!=0)
		return 0;
		
	onion_response *res=onion_response_new(request);
	onion_response_write_headers(res);

	d->render(res);
	
	return onion_response_free(res);
}


void onion_handler_opack_delete(onion_handler_opack_data *data){
	free(data->path);
	free(data);
}

/**
 * @short Creates an path handler. If the path matches the regex, it reomves that from the regexp and goes to the inside_level.
 *
 * If on the inside level nobody answers, it just returns NULL, so ->next can answer.
 */
onion_handler *onion_handler_opack(const char *path, onion_opack_renderer render){
	onion_handler_opack_data *priv_data=malloc(sizeof(onion_handler_opack_data));

	priv_data->path=strdup(path);
	priv_data->render=render;
	
	onion_handler *ret=onion_handler_new((onion_handler_handler)onion_handler_opack_handler,
																			 priv_data, (onion_handler_private_data_free) onion_handler_opack_delete);
	return ret;
}

