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

#include <onion/handler.h>
#include <onion/response.h>

#include "static.h"

struct onion_handler_regexp_data_t{
	regex_t regexp;
	onion_handler *inside;
};

typedef struct onion_handler_regexp_data_t onion_handler_regexp_data;

/**
 * @short Performs the real request: checks if its for me, and then calls the inside level.
 */
int onion_handler_regexp_handler(onion_handler_regexp_data *d, onion_request *request){
	if (regexec(&d->regexp, onion_request_get_path(request), 0, NULL, 0)!=0)
		return 0;

	return onion_handler_handle(d->inside, request);
}

/// Removes internal data for this handler.
void onion_handler_regexp_free(onion_handler_regexp_data *d){
	regfree(&d->regexp);
	onion_handler_free(d->inside);
	free(d);
}

/**
 * @short Creates a static handler that just writes some static data.
 *
 * Path is a regex for the url, as arrived here.
 */
onion_handler *onion_handler_regexp(const char *regexp, onion_handler *next){
	onion_handler_regexp_data *priv_data=calloc(1,sizeof(onion_handler_regexp_data));

	int err=regcomp(&priv_data->regexp, regexp, REG_EXTENDED); // empty regexp, always true. should be fast enough. 
	if (err){
		char buffer[1024];
		regerror(err, &priv_data->regexp, buffer, sizeof(buffer));
		fprintf(stderr, "%s:%d Error analyzing regular expression '%s': %s.\n", __FILE__,__LINE__, regexp, buffer);
		onion_handler_regexp_free(priv_data);
		return NULL;
	}

	onion_handler *ret=onion_handler_new((onion_handler_handler)onion_handler_regexp_handler,
																			 priv_data,(onion_handler_private_data_free) onion_handler_regexp_free);
	return ret;
}

