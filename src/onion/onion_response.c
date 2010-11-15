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
#include <stdio.h>
#include <stdarg.h>
#include <libgen.h>

#include "onion_dict.h"
#include "onion_request.h"
#include "onion_server.h"
#include "onion_response.h"
#include "onion_types_internal.h"

const char *onion_response_code_description(int code);

/// Generates a new response object
onion_response *onion_response_new(onion_request *req){
	onion_response *res=malloc(sizeof(onion_response));
	
	res->request=req;
	res->headers=onion_dict_new();
	res->code=200; // The most normal code, so no need to overwrite it in other codes.
	res->flags=OR_KEEP_ALIVE;
	res->length=res->sent_bytes=0;
	
	// Sorry for the publicity.
	onion_dict_add(res->headers, "Server", "Onion lib - 0.1. http://coralbits.com", 0);
	//time_t t=time(NULL);
	//onion_dict_add(res->headers, "Date", asctime(localtime(&t)), OD_DUP_VALUE);
	
	return res;
}

/// Frees the memory consumed by this object
void onion_response_free(onion_response *res){
	/*
	if (res->flags&OR_LENGTH_SET && res->length==res->sent_bytes){
	}
	*/
	
	onion_dict_free(res->headers);
	free(res);
}

/// Adds a header to the response object
void onion_response_set_header(onion_response *res, const char *key, const char *value){
	onion_dict_add(res->headers, key, value, OD_DUP_ALL); // FIXME. DUP_ALL not so nice on memory side...
}

/// Sets the header length. Normally it should be through set_header, but as its very common and needs some procesing here is a shortcut
void onion_response_set_length(onion_response *res, unsigned int len){
	char tmp[16];
	sprintf(tmp,"%d",len);
	onion_response_set_header(res, "Content-Length", tmp);
	res->length=len;
	res->flags|=OR_LENGTH_SET;
}

/// Sets the return code
void onion_response_set_code(onion_response *res, int  code){
	res->code=code;
}

/// Helper that is called on each header, and writes the header
static void write_header(const char *key, const char *value, onion_response *res){
	onion_response_printf(res, "%s: %s\n",key, value);
}

/// Writes all the header to the given fd
void onion_response_write_headers(onion_response *res){
	onion_response_printf(res, "HTTP/1.1 %d %s\n",res->code, onion_response_code_description(res->code));
	
	onion_dict_preorder(res->headers, write_header, res);
	
	onion_response_write(res,"\n",1);
}

/// Straight write some data to the response. Only used for real data as it has info about sent bytes for keep alive.
int onion_response_write(onion_response *res, const char *data, unsigned int length){
	void *fd=onion_response_get_socket(res);
	onion_write write=onion_response_get_writer(res);
	int w;
	int pos=0;
	while ( (w=write(fd, &data[pos], length)) != length){
		fprintf(stderr, "wrote  %d/%d bytes\n",w,length);
		if (w<=0){
			fprintf(stderr,"%s:%d Error writing. Maybe closed connection. Code %d.\n",basename(__FILE__),__LINE__,w);
#ifdef HAVE_GNUTLS
			fprintf(stderr,"%s:%d %s\n",basename(__FILE__),__LINE__, gnutls_strerror (w));
#endif
			break;
		}
		pos+=w;
		length-=w;
	}
	res->sent_bytes+=length;
	return w;
}

/// Writes a 0-ended string to the response.
int onion_response_write0(onion_response *res, const char *data){
	return onion_response_write(res, data, strlen(data));
}

/// Writes some data to the response. Using sprintf format strings. Max final string size: 1024
int onion_response_printf(onion_response *res, const char *fmt, ...){
	char temp[1024];
	va_list ap;
	va_start(ap, fmt);
	int l=vsnprintf(temp, sizeof(temp)-1, fmt, ap);
	va_end(ap);
	return onion_response_write(res, temp, l);
}



/**
 * Returns the writer method that can be used to write to the socket.
 */
onion_write onion_response_get_writer(onion_response *response){
	return response->request->server->write;
}

/// Returns the socket object.
void *onion_response_get_socket(onion_response *response){
	return response->request->socket;
}

/**
 * @short Shortcut for fast responses, like errors.
 * 
 * Prepares a fast response. You pass only the request, the text and the code, and it do the full response
 * object and sends the data.
 */
int onion_response_shortcut(onion_request *req, const char *response, int code){
	onion_response *res=onion_response_new(req);
	unsigned int l=strlen(response);
	onion_response_set_length(res,l);
	onion_response_set_code(res,code);
	onion_response_write_headers(res);
	
	onion_response_write(res,response,l);
	onion_response_free(res);
	return 1;
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
