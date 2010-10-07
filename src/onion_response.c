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

#include <malloc.h>
#include <string.h>
#include <unistd.h>

#include "onion_response.h"

const char *onion_response_code_description(int code);

/// Generates a new response object
onion_response *onion_response_new(){
	onion_response *res=malloc(sizeof(onion_response));
	
	res->headers=onion_dict_new();
	res->code=200; // The most normal code, so no need to overwrite it in other codes.
	res->flags=OR_KEEP_ALIVE;
	
	return res;
}

/// Frees the memory consumed by this object
void onion_response_free(onion_response *res){
	onion_dict_free(res->headers);
	free(res);
}

/// Adds a header to the response object
void onion_response_set_header(onion_response *res, const char *key, const char *value){
	onion_dict_add(res->headers, key, value, OD_DUP_ALL); // FIXME. DUP_ALL not so nice on memory side...
}

/// Sets the header length. Normally it should be through set_header, but as its very common and needs some procesing here is a shortcut
void onion_response_set_length(onion_response *res, int len){
	char tmp[16];
	sprintf(tmp,"%d",len);
	onion_response_set_header(res, "Content-Length", tmp);
}

/// Sets the return code
void onion_response_set_code(onion_response *res, int  code){
	res->code=code;
}

/// Writes all the header to the given fd
void onion_response_write(onion_response *res, int fd){
#define W(...) { sprintf(tmp, __VA_ARGS__); write(fd, tmp, strlen(tmp)); write(fd,"\n",1); }
	char tmp[1024];

	W("HTTP/1.1 %d %s",res->code, onion_response_code_description(res->code));
	
	// FIXME WRITE ALL HEADERS.
	
#undef W
}

/**
 * @short Returns a const char * string with the code description.
 */
const char *onion_response_code_description(int code){
	switch(code){
		case 200:
			return "OK";
		case 404:
			return "NOT FOUND";
		case 500:
			return "INTERNAL ERROR";
		case 302:
			return "REDIRECT";
	}
	return "INTERNAL ERROR - CODE UNKNOWN";
}
