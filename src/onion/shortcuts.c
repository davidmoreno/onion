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
#include <stdarg.h>

#include "onion.h"
#include "shortcuts.h"

/**
 * @short Shortcut for fast responses, like errors.
 * 
 * Prepares a fast response. You pass only the request, the text and the code, and it do the full response
 * object and sends the data.
 */
int onion_shortcut_response(onion_request* req, const char* response, int code)
{
	return onion_shortcut_response_extra_headers(req, response, code, NULL);
}


/**
 * @short Shortcut for fast responses, like errors, with extra headers
 * 
 * Prepares a fast response. You pass only the request, the text and the code, and it do the full response
 * object and sends the data.
 * 
 * On this version you also pass a NULL terminated list of headers, in key, value pairs.
 */
int onion_shortcut_response_extra_headers(onion_request* req, const char* response, int code, ... ){
	onion_response *res=onion_response_new(req);
	unsigned int l=strlen(response);
	const char *key, *value;
	
	onion_response_set_length(res,l);
	onion_response_set_code(res,code);
	
	va_list ap;
	va_start(ap, code);
	while ( (key=va_arg(ap, const char *)) ){
		value=va_arg(ap, const char *);
		if (key && value)
			onion_response_set_header(res, key, value);
		else
			break;
	}
	va_end(ap);

	onion_response_write_headers(res);
	
	onion_response_write(res,response,l);
	return onion_response_free(res);
}


/**
 * @short Shortcut to ease a redirect. 
 * 
 * You can add a message for non-compilant browsers, and the new url to which you redirect.
 */
int onion_shortcut_redirect(onion_request *req, const char *response, const char *newurl){
	return onion_shortcut_response_extra_headers(req, response ? response : "<h1>302 - Moved</h1>",
																							 HTTP_REDIRECT, "Location", newurl, NULL );
}
