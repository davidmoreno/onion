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

#include "log.h"
#include "handler.h"
#include "response.h"
#include "url.h"
#include "types_internal.h"
#include "dict.h"
#include "low.h"

enum onion_url_data_flags_e{
	OUD_REGEXP=1,
	OUD_STRCMP=2,
};

typedef enum onion_url_data_flags_e onion_url_data_flags;

/**
 * @short Internal onion_url data for each known url
 * @private
 */
struct onion_url_data_t{
	union{
		regex_t regexp;
		char *str;
	};
#ifdef __DEBUG__
	char *orig;
#endif
	int flags;
	onion_handler *inside;
	struct onion_url_data_t *next;
};

//typedef struct onion_url_data_t onion_url_data; // already at types-internal.h

/**
 * @short Performs the real request: checks if its for me, and then calls the inside level.
 * @ingroup url
 */
int onion_url_handler(onion_url_data **dd, onion_request *request, onion_response *response){
	onion_url_data *next=*dd;
	regmatch_t match[16];
	int i;

	const char *path=onion_request_get_path(request);
	while (next){
		ONION_DEBUG0("Check %s against %s", onion_request_get_path(request), next->orig);
		if (next->flags&OUD_STRCMP){
			if (strcmp(path, next->str)==0){
				ONION_DEBUG0("Ok, simple match.");

				onion_request_advance_path(request, strlen(next->str));
				return onion_handler_handle(next->inside, request, response);
			}
		}
		else if (regexec(&next->regexp, onion_request_get_path(request), 16, match, 0)==0){
			//ONION_DEBUG("Ok,match");
			onion_dict *reqheader=request->GET;
			for (i=1;i<16;i++){
				regmatch_t *rm=&match[i];
				if (rm->rm_so!=-1){
					char *tmp=onion_low_scalar_malloc(rm->rm_eo-rm->rm_so+1);
					memcpy(tmp, &path[rm->rm_so], rm->rm_eo-rm->rm_so);
					tmp[rm->rm_eo-rm->rm_so]='\0'; // proper finish string
					char tmpn[4];
					snprintf(tmpn,sizeof(tmpn),"%d",i);
					onion_dict_add(reqheader, tmpn, tmp, OD_DUP_KEY|OD_FREE_VALUE);
					ONION_DEBUG("Add group %d: %s (%d-%d)", i, tmp, rm->rm_so, rm->rm_eo);
				}
				else
					break;
			}
			onion_request_advance_path(request, match[0].rm_eo);
			ONION_DEBUG0("Ok, regexp match.");


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
		onion_handler_free(t->inside);
		if (t->flags&OUD_REGEXP)
			regfree(&t->regexp);
		else
			onion_low_free(t->str);
		next=t->next;
#ifdef __DEBUG__
		onion_low_free(t->orig);
#endif
		onion_low_free(t);
	}
	onion_low_free(d);
}

/**
 * @short Creates the URL handler to map regex urls to handlers
 * @ingroup url
 *
 * The onion_url object can be used to add urls as needed using onion_url_add_*.
 *
 * The URLs can be regular expressions or simple strings. The way to discriminate them is just
 * to check the first character; if its ^ its a regular expression.
 *
 * If a string is passed then the full path must match. If its a regexp, just the begining is matched,
 * unless $ is set at the end. When matched, this is removed from the path.
 *
 * It is important to note that when the user pretends to match the initial path elements, to later
 * pass to another handler that will do path transversal (another url object, for example), the
 * path must be written with a regular expression, for example: "^login/". If the user just writes
 * the string it will match only for that specific URL, and subpaths are not in the definition.
 *
 * When looking for a match, they are looked in order.
 *
 * Examples::
 *
 * @code
 *  onion_url_add(url, "index.html", index); // Matches the exact filename. Not compiled.
 *  onion_url_add(url, "^static/", onion_handler_export_local_new(".") ); // Export current directory at static
 *  onion_url_add(url, "^icons/(.*)", directory); // Compiles the regexp, and uses the .* as first argument.
 *  onion_url_add(url, "", redirect_to_index); // Matches an empty path. Not compiled.
 * @endcode
 *
 * Regexp can have groups, and they will be added as request query parameters, with just the number of the
 * group as key. The groups start at 1, as 0 should be the full match, but its not added for performance
 * reasons; its a very strange situation that user will need it, and always can access full path with
 * onion_request_get_fullpath. Also all expression can be a group, and passed as nr 1.:
 *
 * @code
 *  onion_url_add(url, "^index(.html)", index);
 *  ...
 *  onion_request_get_query(req, "1") == ".html"
 * @endcode
 *
 * Be careful as . means every character, and dots in URLs must be with a backslash \ (double because of
 * C escaping), if using regexps.
 *
 * Regular expressions are used by the regexec(3) standard C library. Check that documentation to check
 * how to create proper regular expressions. They are compiled as REG_EXTENDED.
 */
onion_url *onion_url_new(){
	onion_url_data **priv_data=onion_low_calloc(1,sizeof(onion_url_data*));
	*priv_data=NULL;

	onion_handler *ret=onion_handler_new((onion_handler_handler)onion_url_handler,
																			 priv_data,(onion_handler_private_data_free) onion_url_free_data);
	return (onion_url*)ret;
}

/**
 * @short Frees the url.
 * @ingroup url
 */
void onion_url_free(onion_url* url){
	onion_handler_free((onion_handler*)url);
}



/**
 * @short Adds a new handler with the given regexp.
 * @ingroup url
 *
 * Adds the given handler.
 *
 * @returns 0 if everything ok. Else there is a regexp error.
 */
int onion_url_add_handler(onion_url *url, const char *regexp, onion_handler *next){
	onion_url_data **w=((onion_url_data**)onion_handler_get_private_data((onion_handler*)url));
	while (*w){
		w=&(*w)->next;
	}
	//ONION_DEBUG("Adding handler at %p",w);
	*w=onion_low_malloc(sizeof(onion_url_data));
	onion_url_data *data=*w;

	data->flags=(regexp[0]=='^') ? OUD_REGEXP : OUD_STRCMP;

	if (data->flags&OUD_REGEXP){
		int err=regcomp(&data->regexp, regexp, REG_EXTENDED); // empty regexp, always true. should be fast enough.
		if (err){
			char buffer[1024];
			regerror(err, &data->regexp, buffer, sizeof(buffer));
			ONION_ERROR("Error analyzing regular expression '%s': %s.\n", regexp, buffer);
			onion_low_free(data);
			*w=NULL;
			return 1;
		}
	}
	else{
		data->str=onion_low_strdup(regexp);
	}
	data->next=NULL;
	data->inside=next;
#ifdef __DEBUG__
	data->orig=onion_low_strdup(regexp);
#endif

	return 0;
}

/**
 * @short Helper to simple add basic handlers
 * @ingroup url
 *
 * @returns 0 if everything ok. Else there is a regexp error.
 */
int onion_url_add(onion_url *url, const char *regexp, void *handler){
	return onion_url_add_handler(url, regexp, onion_handler_new(handler, NULL, NULL));
}

/**
 * @short Helper to simple add a basic handler with data
 * @ingroup url
 *
 * @returns 0 if everything ok. Else there is a regexp error.
 */
int onion_url_add_with_data(onion_url *url, const char *regexp, void *handler, void *data, void *f){
	return onion_url_add_handler(url, regexp, onion_handler_new(handler, data, f));
}

/**
 * @short Adds a regex url, with another url as handler.
 * @ingroup url
 *
 * @returns 0 if everything ok. Else there is a regexp error.
 */
int onion_url_add_url(onion_url *url, const char *regexp, onion_url *handler){
	return onion_url_add_handler(url, regexp, (onion_handler*) handler);
}

/**
 * @short Simple data needed for static data write
 * @private
 */
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
	onion_low_free(data->text);
	onion_low_free(data);
}

/**
 * @short Adds a simple handler, it has static data and a default return code
 * @ingroup url
 */
int onion_url_add_static(onion_url *url, const char *regexp, const char *text, int http_code){
	struct onion_url_static_data *d=onion_low_malloc(sizeof(struct onion_url_static_data));
	d->text=onion_low_strdup(text);
	d->code=http_code;
	return onion_url_add_with_data(url, regexp, onion_url_static, d, onion_url_static_free);
}

/**
 * @short Returns the related handler for this url object.
 * @ingroup url
 */
onion_handler *onion_url_to_handler(onion_url *url){
	return ((onion_handler*)url);
}
