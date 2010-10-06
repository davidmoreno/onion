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

#include "onion_handler_static.h"

struct onion_handler_static_data_t{
	int code;
	const char *data;
};

typedef struct onion_handler_static_data_t onion_handler_static_data;

void onion_handler_static_generator(onion_response *response, char *data, unsigned int max_length){
	onion_handler_static_data *d=response->priv_data;
	strncpy(data, d->data, max_length);
}

onion_response *onion_handler_static_parser(onion_handler *handler, onion_request *request){
	onion_response *res=malloc(sizeof(onion_response));
	memset(res,0,sizeof(res));
	onion_handler_static_data *d=handler->priv_data;

	res->code=d->code;
	res->data_generator=onion_handler_static_generator;
	res->headers=onion_dict_new();
	res->priv_data=d;
	res->priv_data_delete=NULL;
	
	char s[256];
	sprintf(s,"%ld",strlen(d->data));
	onion_dict_add(res->headers, "Content-Length", s, OD_DUP_VALUE);
	return res;
}


void onion_handler_static_delete(void *data){
	onion_handler_static_data *d=data;
	free((char*)d->data);
	free(d);
}

onion_handler *onion_handler_static(const char *text, int code){
	onion_handler *ret;
	ret=malloc(sizeof(onion_handler));
	memset(ret,0,sizeof(onion_handler));
	
	onion_handler_static_data *priv_data=malloc(sizeof(onion_handler_static_data));
	priv_data->code=code;
	priv_data->data=strdup(text);
	
	ret->parser=onion_handler_static_parser;
	ret->priv_data=priv_data;
	ret->priv_data_delete=onion_handler_static_delete;
	
	return ret;
}

