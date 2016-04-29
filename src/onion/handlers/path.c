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
#include <stdio.h>

#include <onion/handler.h>
#include <onion/response.h>
#include <onion/log.h>
#include <onion/low.h>

#include "static.h"

struct onion_handler_path_data_t{
	regex_t path;
	onion_handler *inside;
};

typedef struct onion_handler_path_data_t onion_handler_path_data;

int onion_handler_path_handler(onion_handler_path_data *d, onion_request *request, onion_response *response){
	regmatch_t match[1];
	const char *path=onion_request_get_path(request);
	
	ONION_DEBUG("Check path %s", path);
	
	if (regexec(&d->path, path, 1, match, 0)!=0)
		return 0;
	
	onion_request_advance_path(request, match[0].rm_eo);
	
	return onion_handler_handle(d->inside, request, response);
}


void onion_handler_path_delete(void *data){
	onion_handler_path_data *d=data;
	regfree(&d->path);
	onion_handler_free(d->inside);
	onion_low_free(data);
}

/**
 * @short Creates an path handler. If the path matches the regex, it reomves that from the regexp and goes to the inside_level.
 *
 * If on the inside level nobody answers, it just returns NULL, so ->next can answer.
 */
onion_handler *onion_handler_path(const char *path, onion_handler *inside_level){
	onion_handler_path_data *priv_data=onion_low_malloc(sizeof(onion_handler_path_data));
	if (!priv_data)
		return NULL;
	
	priv_data->inside=inside_level;
	
	// Path is a little bit more complicated, its an regexp.
	int err;
	if (path)
		err=regcomp(&priv_data->path, path, REG_EXTENDED);
	else
		err=regcomp(&priv_data->path, "", REG_EXTENDED); // empty regexp, always true. should be fast enough. 
	if (err){
		char buffer[1024];
		regerror(err, &priv_data->path, buffer, sizeof(buffer));
		fprintf(stderr, "%s:%d Error analyzing regular expression '%s': %s.\n", __FILE__,__LINE__, path, buffer);
		onion_handler_path_delete(priv_data);
		return NULL;
	}

	onion_handler *ret=onion_handler_new((onion_handler_handler)onion_handler_path_handler,
																			 priv_data, (onion_handler_private_data_free) onion_handler_path_delete);
	return ret;
}

