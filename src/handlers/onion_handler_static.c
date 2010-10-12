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

#include <onion_response.h>

#include "onion_handler_static.h"

struct onion_handler_static_data_t{
	int code;
	const char *data;
};

typedef struct onion_handler_static_data_t onion_handler_static_data;

int onion_handler_static_handler(onion_handler *handler, onion_request *request){
	onion_handler_static_data *d=handler->priv_data;
	onion_response *res=onion_response_new(request);

	int length=strlen(d->data);
	onion_response_set_length(res, length);
	onion_response_set_code(res, d->code);
	
	onion_response_write_headers(res);
	
	onion_response_write(res, d->data, length);

	onion_response_free(res);
	
	return 1;
}


void onion_handler_static_delete(void *data){
	onion_handler_static_data *d=data;
	free((char*)d->data);
	free(d);
}

/**
 * @short Creates a static handler that just writes some static data.
 */
onion_handler *onion_handler_static(const char *text, int code){
	onion_handler *ret;
	ret=malloc(sizeof(onion_handler));
	memset(ret,0,sizeof(onion_handler));
	
	onion_handler_static_data *priv_data=malloc(sizeof(onion_handler_static_data));
	priv_data->code=code;
	priv_data->data=strdup(text);
	
	ret->handler=onion_handler_static_handler;
	ret->priv_data=priv_data;
	ret->priv_data_delete=onion_handler_static_delete;
	
	return ret;
}

