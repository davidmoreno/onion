/*
	Onion HTTP server library
	Copyright (C) 2010-2011 David Moreno Montero

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

#include "log.h"
#include "handler.h"
#include "response.h"
#include "url.h"


struct onion_url_data_t{
	regex_t regexp;
	char *orig;
	onion_handler *inside;
	struct onion_url_data_t *next;
};

typedef struct onion_url_data_t onion_url_data;

/**
 * @short Performs the real request: checks if its for me, and then calls the inside level.
 */
int onion_url_handler(onion_url_data **dd, onion_request *request, onion_response *response){
	onion_url_data *next=*dd;
	regmatch_t match[1];
	
	while (next){
		//ONION_DEBUG("Check %s against %s", onion_request_get_path(request), next->orig);
		if (regexec(&next->regexp, onion_request_get_path(request), 1, match, 0)==0){
			//ONION_DEBUG("Ok,match");
			if (match[0].rm_so==0)
				onion_request_advance_path(request, match[0].rm_eo);
			return onion_handler_handle(next->inside, request, response);
		}
		
		next=next->next;
	}
	return 0;
}

/// Removes internal data for this handler.
void onion_url_free_data(onion_url_data **d){
	onion_url_data *next=*d;
	while (next){
		onion_url_data *t=next;
		regfree(&t->regexp);
		onion_handler_free(t->inside);
		free(t->orig);
		next=t->next;
		free(t);
	}
	free(d);
}

/**
 * @short Creates a static handler that just writes some static data.
 *
 * Path is a regex for the url, as arrived here.
 */
onion_url *onion_url_new(){
	onion_url_data **priv_data=calloc(1,sizeof(onion_url_data*));
	*priv_data=NULL;
	
	onion_handler *ret=onion_handler_new((onion_handler_handler)onion_url_handler,
																			 priv_data,(onion_handler_private_data_free) onion_url_free_data);
	return (onion_url*)ret;
}

/// Frees the url.
void onion_url_free(onion_url* url){
	onion_handler_free((onion_handler*)url);
}



/**
 * @short Adds a new handler with the given regexp.
 * 
 * When looking for a match, they are looked in order.
 * 
 * If the the found regexp start at begining, the path component of the request will advance. 
 * This is the normal usage, as normally you will ask for a regexp starting with ^.
 * 
 * Examples::
 * 
 * @code
 *  onion_url_add(url, "^index.html$", index);
 *  onion_url_add(url, "^icons/", directory); // No end $, so directory will get the path without the icons/
 *  onion_url_add(url, "^$", redirect_to_index);
 * @endcode
 * 
 * @returns 0 if everything ok. Else its a regexp error.
 */
int onion_url_add_handler(onion_url *url, const char *regexp, onion_handler *next){
	onion_url_data **w=((onion_url_data**)onion_handler_get_private_data((onion_handler*)url));
	while (*w){
		w=&(*w)->next;
	}
	//ONION_DEBUG("Adding handler at %p",w);
	*w=malloc(sizeof(onion_url_data));
	onion_url_data *data=*w;
	
	int err=regcomp(&data->regexp, regexp, REG_EXTENDED); // empty regexp, always true. should be fast enough. 
	if (err){
		char buffer[1024];
		regerror(err, &data->regexp, buffer, sizeof(buffer));
		ONION_ERROR("Error analyzing regular expression '%s': %s.\n", regexp, buffer);
		free(data);
		*w=NULL;
		return 1;
	}
	data->orig=strdup(regexp);
	data->next=NULL;
	data->inside=next;
	
	return 0;
}

/**
 * @short Helper to simple add basic handlers
 */
int onion_url_add(onion_url *url, const char *regexp, void *handler){
	return onion_url_add_handler(url, regexp, onion_handler_new(handler, NULL, NULL));
}

/**
 * @short Helper to simple add a basic handler with data
 */
int onion_url_add_with_data(onion_url *url, const char *regexp, void *handler, void *data, void *f){
	return onion_url_add_handler(url, regexp, onion_handler_new(handler, data, f));
}

/**
 * @short Adds a regex url, with another url as handler.
 */
int onion_url_add_url(onion_url *url, const char *regexp, onion_url *handler){
	return onion_url_add_handler(url, regexp, (onion_handler*) handler);
}

/// Simple data needed for static data write
struct onion_url_static_data{
	char *text;
	int code;
};

/// Handles the write of static data
static int onion_url_static(struct onion_url_static_data *data, onion_request *req, onion_response *res){
	onion_response_set_code(res, data->code);
	onion_response_set_length(res, strlen(data->text));
	onion_response_write0(res, data->text);
	return OCS_PROCESSED;
}

/// Frees the static data
static void onion_url_static_free(struct onion_url_static_data *data){
	free(data->text);
	free(data);
}

/**
 * @short Adds a simple handler, it has static data and a default return code
 */
int onion_url_add_static(onion_url *url, const char *regexp, const char *text, int http_code){
	struct onion_url_static_data *d=malloc(sizeof(struct onion_url_static_data));
	d->text=strdup(text);
	d->code=http_code;
	return onion_url_add_with_data(url, regexp, onion_url_static, d, onion_url_static_free);
}

/**
 * @short Returns the related handler for this url object.
 */
onion_handler *onion_url_to_handler(onion_url *url){
	return ((onion_handler*)url);
}
