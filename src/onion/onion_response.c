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

#include "onion_dict.h"
#include "onion_request.h"
#include "onion_server.h"
#include "onion_response.h"
#include "onion_types_internal.h"
#include "onion_log.h"

const char *onion_response_code_description(int code);
static void onion_response_write_buffer(onion_response *res);

/// Generates a new response object
onion_response *onion_response_new(onion_request *req){
	onion_response *res=malloc(sizeof(onion_response));
	
	res->request=req;
	res->headers=onion_dict_new();
	res->code=200; // The most normal code, so no need to overwrite it in other codes.
	res->flags=0;
	res->sent_bytes_total=res->length=res->sent_bytes=0;
	res->buffer_pos=0;
	
	// Sorry for the publicity.
	onion_dict_add(res->headers, "Server", "libonion v0.1 - coralbits.com", 0);
	//time_t t=time(NULL);
	//onion_dict_add(res->headers, "Date", asctime(localtime(&t)), OD_DUP_VALUE);
	
	return res;
}

/**
 * @short Frees the memory consumed by this object
 * 
 * This function returns the close status: OR_KEEP_ALIVE or OR_CLOSE_CONNECTION as needed.
 */
int onion_response_free(onion_response *res){
	// write pending data.
	onion_response_write_buffer(res);
	
	int r=OR_CLOSE_CONNECTION;
	// keep alive only on HTTP/1.1.
	if (res->request->flags&OR_HTTP11 && (res->flags&OR_SKIP_CONTENT || (res->flags&OR_KEEP_ALIVE && res->length==res->sent_bytes)))
		r=OR_KEEP_ALIVE;
	
	// FIXME! This is no proper logging at all. Maybe use a handler.
	ONION_INFO("[%s] \"%s %s\" %d %d (%s)", res->request->client_info, (res->request->flags&OR_GET) ? "GET" : (res->request->flags&OR_HEAD) ? "HEAD" : (res->request->flags&OR_POST) ? "POST" : "UNKNOWN_METHOD",
					res->request->fullpath, res->code, res->sent_bytes,
					(r==OR_KEEP_ALIVE) ? "Keep-Alive" : "Close connection");
	
	onion_dict_free(res->headers);
	free(res);
	
	return r;
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
	const char *connection=onion_request_get_header(res->request,"Connection");
	if (!connection || strcasecmp(connection,"Close")!=0){ // Other side wants keep alive
		//onion_response_set_header(res, "Connection","Keep-Alive");  // Unnecesary on HTTP/1.1
		res->flags|=OR_KEEP_ALIVE;
	}
}

/// Sets the return code
void onion_response_set_code(onion_response *res, int  code){
	res->code=code;
}

/// Helper that is called on each header, and writes the header
static void write_header(const char *key, const char *value, onion_response *res){
	ONION_DEBUG0("Response header: %s: %s",key, value);

	onion_response_printf(res, "%s: %s\n",key, value);
}

#define CONNECTION_CLOSE "Connection: Close\n"

/**
 * @short Writes all the header to the given response
 * 
 * It writes the headers and depending on the method, return OR_SKIP_CONTENT. this is set when in head mode. Handlers 
 * should react to this return by not trying to write more, but if they try this object will just skip those writings.
 * 
 * @returns 0 if should procced to normal data write, or OR_SKIP_CONTENT if should not write content.
 */
int onion_response_write_headers(onion_response *res){
	onion_response_printf(res, "HTTP/1.1 %d %s\n",res->code, onion_response_code_description(res->code));
	
	if (!(res->flags&OR_LENGTH_SET))
		onion_response_write(res, CONNECTION_CLOSE, sizeof(CONNECTION_CLOSE)-1);
	
	onion_dict_preorder(res->headers, write_header, res);
	
	onion_response_write(res,"\n",1);
	
	res->sent_bytes=0; // the header size is not counted here.
		
	if (res->request->flags&OR_HEAD){
		res->flags|=OR_SKIP_CONTENT;
		return OR_SKIP_CONTENT;
	}
	return 0;
}

/// Straight write some data to the response. Only used for real data as it has info about sent bytes for keep alive.
int onion_response_write(onion_response *res, const char *data, unsigned int length){
	if (res->flags&OR_SKIP_CONTENT){
		ONION_DEBUG("Skipping content as we are in HEAD mode");
		return -1;
	}
	
	res->sent_bytes+=length;
	res->sent_bytes_total+=length;
	
	while (res->buffer_pos+length>sizeof(res->buffer)){
		int wb=sizeof(res->buffer)-res->buffer_pos;
		memcpy(&res->buffer[res->buffer_pos], data, wb);
		
		res->buffer_pos=sizeof(res->buffer);
		onion_response_write_buffer(res);
		
		length-=wb;
		data+=wb;
	}
	
	memcpy(&res->buffer[res->buffer_pos], data, length);
	res->buffer_pos+=length;
	
	return length;
}

/// Writes all buffered output waiting for sending.
static void onion_response_write_buffer(onion_response *res){
	void *fd=onion_response_get_socket(res);
	onion_write write=res->request->server->write;
	int w;
	int pos=0;
	ONION_DEBUG("Write %d bytes",res->buffer_pos);
	while ( (w=write(fd, &res->buffer[pos], res->buffer_pos)) != res->buffer_pos){
		if (w<=0){
			ONION_ERROR("Error writing at %d. Maybe closed connection. Code %d. ",res->buffer_pos, w);
			perror("");
			res->buffer_pos=0;
			return;
		}
		pos+=w;
		res->buffer_pos-=w;
		ONION_DEBUG("Write %d bytes",res->buffer_pos);
	}
	res->buffer_pos=0;
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
	return onion_response_free(res);
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
