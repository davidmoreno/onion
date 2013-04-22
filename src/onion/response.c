/*
	Onion HTTP server library
	Copyright (C) 2010-2013 David Moreno Montero

	This library is free software; you can redistribute it and/or
	modify it under the terms of, at your choice:
	
	a. the GNU Lesser General Public License as published by the 
	 Free Software Foundation; either version 3.0 of the License, 
	 or (at your option) any later version.
	
	b. the GNU General Public License as published by the 
	 Free Software Foundation; either version 2.0 of the License, 
	 or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License and the GNU General Public License along with this 
	library; if not see <http://www.gnu.org/licenses/>.
	*/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>

#include "dict.h"
#include "request.h"
#include "response.h"
#include "types_internal.h"
#include "log.h"

const char *onion_response_code_description(int code);
static int onion_response_write_buffer(onion_response *res);

/**
 * @short Generates a new response object
 * @memberof onion_response_t
 * 
 * This response is generated from a request, and gets from there the writer and writer data.
 * 
 * Also fills some important data, as server Id, License type, and the default content type.
 * 
 * Default content type is HTML, as normally this is what is needed. This is nontheless just 
 * the default, and can be changed to any other with a call to:
 * 
 *   onion_response_set_header(res, "Content-Type", my_type);
 * 
 * The response object must be freed with onion_response_free, which also returns the keep alive 
 * status.
 * 
 * onion_response objects are passed by onion internally to process the request, and should not be
 * created by user normally. Nontheless the option exist.
 * 
 * @returns An onion_response object for that request.
 */
onion_response *onion_response_new(onion_request *req){
	onion_response *res=malloc(sizeof(onion_response));
	
	res->request=req;
	res->headers=onion_dict_new();
	res->code=200; // The most normal code, so no need to overwrite it in other codes.
	res->flags=0;
	res->sent_bytes_total=res->length=res->sent_bytes=0;
	res->buffer_pos=0;
	
	// Sorry for the publicity.
	onion_dict_add(res->headers, "Server", "libonion v0.5 - coralbits.com", 0);
	onion_dict_add(res->headers, "Content-Type", "text/html", 0); // Maybe not the best guess, but really useful.
	//time_t t=time(NULL);
	//onion_dict_add(res->headers, "Date", asctime(localtime(&t)), OD_DUP_VALUE);
	
	return res;
}

/**
 * @short Frees the memory consumed by this object
 * @memberof onion_response_t
 * 
 * This function returns the close status: OR_KEEP_ALIVE or OR_CLOSE_CONNECTION as needed.
 * 
 * @returns Whether the connection should be closed or not, or an error status to be handled by server.
 * @see onion_connection_status
 */
onion_connection_status onion_response_free(onion_response *res){
	// write pending data.
	onion_response_write_buffer(res);
	onion_request *req=res->request;
	
	if (res->flags&OR_CHUNKED){ // Set the chunked data end.
		req->connection.listen_point->write(req, "0\r\n\r\n",5);
	}
	
	int r=OCS_CLOSE_CONNECTION;
	
	// it is a rare ocassion that there is no request, but although unlikely, it may happend
	if (req){
		// keep alive only on HTTP/1.1.
		//ONION_DEBUG("keep alive [req wants] %d && ([skip] %d || [lenght ok] %d || [chunked] %d)", 
		//						onion_request_keep_alive(req),
		//						res->flags&OR_SKIP_CONTENT,res->length==res->sent_bytes, res->flags&OR_CHUNKED);
		if ( onion_request_keep_alive(req) && 
				 ( res->flags&OR_SKIP_CONTENT || res->length==res->sent_bytes || res->flags&OR_CHUNKED ) 
			 )
			r=OCS_KEEP_ALIVE;
		
		// FIXME! This is no proper logging at all. Maybe use a handler.
		ONION_INFO("[%s] \"%s %s\" %d %d (%s)", onion_request_get_client_description(res->request),
							 onion_request_methods[res->request->flags&OR_METHODS],
						res->request->fullpath, res->code, res->sent_bytes,
						(r==OCS_KEEP_ALIVE) ? "Keep-Alive" : "Close connection");
	}
	
	onion_dict_free(res->headers);
	free(res);
	
	return r;
}

/**
 * @short Adds a header to the response object
 * @memberof onion_response_t
 */
void onion_response_set_header(onion_response *res, const char *key, const char *value){
	ONION_DEBUG0("Adding header %s = %s", key, value);
	onion_dict_add(res->headers, key, value, OD_DUP_ALL|OD_REPLACE); // DUP_ALL not so nice on memory side...
}

/**
 * @short Sets the header length. Normally it should be through set_header, but as its very common and needs some procesing here is a shortcut
 * @memberof onion_response_t
 */
void onion_response_set_length(onion_response *res, size_t len){
	char tmp[16];
	sprintf(tmp,"%lu",(unsigned long)len);
	onion_response_set_header(res, "Content-Length", tmp);
	res->length=len;
	res->flags|=OR_LENGTH_SET;
}

/**
 * @short Sets the return code
 * @memberof onion_response_t
 */
void onion_response_set_code(onion_response *res, int  code){
	res->code=code;
}

/**
 * @short Helper that is called on each header, and writes the header
 * @memberof onion_response_t
 */
static void write_header(onion_response *res, const char *key, const char *value, int flags){
	//ONION_DEBUG0("Response header: %s: %s",key, value);

	onion_response_write0(res, key);
	onion_response_write(res, ": ",2);
	onion_response_write0(res, value);
	onion_response_write(res, "\r\n",2);
}

#define CONNECTION_CLOSE "Connection: Close\r\n"
#define CONNECTION_KEEP_ALIVE "Connection: Keep-Alive\r\n"
#define CONNECTION_CHUNK_ENCODING "Transfer-Encoding: chunked\r\n"
#define CONNECTION_UPGRADE "Connection: Upgrade\r\n"

/**
 * @short Writes all the header to the given response
 * @memberof onion_response_t
 * 
 * It writes the headers and depending on the method, return OR_SKIP_CONTENT. this is set when in head mode. Handlers 
 * should react to this return by not trying to write more, but if they try this object will just skip those writtings.
 * 
 * Explicit calling to this function is not necessary, as as soon as the user calls any write function this will 
 * be performed. 
 * 
 * As soon as the headers are written, any modification on them will be just ignored.
 * 
 * @returns 0 if should procced to normal data write, or OR_SKIP_CONTENT if should not write content.
 */
int onion_response_write_headers(onion_response *res){
	res->flags|=OR_HEADER_SENT; // I Set at the begining so I can do normal writing.
	res->request->flags|=OR_HEADER_SENT;
	char chunked=0;
	
	if (res->request->flags&OR_HTTP11){
		onion_response_printf(res, "HTTP/1.1 %d %s\r\n",res->code, onion_response_code_description(res->code));
		//ONION_DEBUG("Response header: HTTP/1.1 %d %s\n",res->code, onion_response_code_description(res->code));
		if (!(res->flags&OR_LENGTH_SET)  && onion_request_keep_alive(res->request)){
			onion_response_write(res, CONNECTION_CHUNK_ENCODING, sizeof(CONNECTION_CHUNK_ENCODING)-1);
			chunked=1;
		}
	}
	else{
		onion_response_printf(res, "HTTP/1.0 %d %s\r\n",res->code, onion_response_code_description(res->code));
		//ONION_DEBUG("Response header: HTTP/1.0 %d %s\n",res->code, onion_response_code_description(res->code));
		if (res->flags&OR_LENGTH_SET) // On HTTP/1.0, i need to state it. On 1.1 it is default.
			onion_response_write(res, CONNECTION_KEEP_ALIVE, sizeof(CONNECTION_KEEP_ALIVE)-1);
	}
	
	if (!(res->flags&OR_LENGTH_SET) && !chunked && !(res->flags&OR_CONNECTION_UPGRADE))
		onion_response_write(res, CONNECTION_CLOSE, sizeof(CONNECTION_CLOSE)-1);
	
	if (res->flags&OR_CONNECTION_UPGRADE)
		onion_response_write(res, CONNECTION_UPGRADE, sizeof(CONNECTION_UPGRADE)-1);
	
	onion_dict_preorder(res->headers, write_header, res);
	
	if (res->request->session_id && (onion_dict_count(res->request->session)>0)) // I have session with something, tell user
		onion_response_printf(res, "Set-Cookie: sessionid=%s; httponly\n", res->request->session_id);
  
	onion_response_write(res,"\r\n",2);
	
	res->sent_bytes=0; // the header size is not counted here.
	
	if ((res->request->flags&OR_METHODS)==OR_HEAD){
		onion_response_write_buffer(res);
		res->flags|=OR_SKIP_CONTENT;
		return OR_SKIP_CONTENT;
	}
	if (chunked){
		onion_response_write_buffer(res);
		res->flags|=OR_CHUNKED;
	}
	
	return 0;
}

/**
 * @short Write some response data.
 * @memberof onion_response_t
 * 
 * This is the main write data function. If the headers have not been sent yet, they are now.
 * 
 * It's internally used also by the write0 and printf versions.
 * 
 * Also it does some buffering, so data is not sent as written by code, but only in chunks. 
 * These chunks are when the response is finished, or when the internal buffer is full. This
 * helps performance, and eases the programming on the user side.
 * 
 * If length is 0, forces the write of pending data.
 * 
 * @returns The bytes written, normally just length. On error returns OCS_CLOSE_CONNECTION.
 */
ssize_t onion_response_write(onion_response *res, const char *data, size_t length){
	if (!(res->flags&OR_HEADER_SENT)){ // Automatic header write
		onion_response_write_headers(res);
	}
	if (res->flags&OR_SKIP_CONTENT){
		ONION_DEBUG("Skipping content as we are in HEAD mode");
		return OCS_CLOSE_CONNECTION;
	}
	if (length==0){
		onion_response_write_buffer(res);
		return 0;
	}
	res->sent_bytes+=length;
	res->sent_bytes_total+=length;

	int l=length;
	int w=0;
	while (res->buffer_pos+l>sizeof(res->buffer)){
		int wb=sizeof(res->buffer)-res->buffer_pos;
		memcpy(&res->buffer[res->buffer_pos], data, wb);
		
		res->buffer_pos=sizeof(res->buffer);
		if (onion_response_write_buffer(res)<0)
			return w;
		
		l-=wb;
		data+=wb;
		w+=wb;
	}
	
	memcpy(&res->buffer[res->buffer_pos], data, l);
	res->buffer_pos+=l;
	w+=l;
	
	return w;
}

/// Writes all buffered output waiting for sending.
static int onion_response_write_buffer(onion_response *res){
	if (res->flags&OR_SKIP_CONTENT || res->buffer_pos==0)
		return 0;
	
	onion_request *req=res->request;
	ssize_t (*write)(onion_request *, const char *data, size_t len);
	write=req->connection.listen_point->write;
	
	ssize_t w;
	off_t pos=0;
	//ONION_DEBUG0("Write %d bytes",res->buffer_pos);
	if (res->flags&OR_CHUNKED){
		char tmp[16];
		snprintf(tmp,sizeof(tmp),"%X\r\n",(unsigned int)res->buffer_pos);
		if (write(req, tmp, strlen(tmp))<=0){
			ONION_WARNING("Error writing chunk encoding length. Aborting write.");
			return OCS_CLOSE_CONNECTION;
		}
	}
	while ( (w=write(req, &res->buffer[pos], res->buffer_pos)) != res->buffer_pos){
		if (w<=0 || res->buffer_pos<0){
			ONION_ERROR("Error writing %d bytes. Maybe closed connection. Code %d. ",res->buffer_pos, w);
			perror("");
			res->buffer_pos=0;
			return OCS_CLOSE_CONNECTION;
		}
		pos+=w;
		ONION_DEBUG0("Write %d-%d bytes",res->buffer_pos,w);
		res->buffer_pos-=w;
	}
	if (res->flags&OR_CHUNKED){
		write(req,"\r\n",2);
	}
	res->buffer_pos=0;
	return 0;
}

/// Writes a 0-ended string to the response.
ssize_t onion_response_write0(onion_response *res, const char *data){
	return onion_response_write(res, data, strlen(data));
}

/**
 * @short Writes some data to the response. Using sprintf format strings. Max final string size: 1024
 * @memberof onion_response_t
 */
ssize_t onion_response_printf(onion_response *res, const char *fmt, ...){
	char temp[1024];
	va_list ap;
	va_start(ap, fmt);
	int l=vsnprintf(temp, sizeof(temp)-1, fmt, ap);
	va_end(ap);
	return onion_response_write(res, temp, l);
}




/**
 * @short Returns a const char * string with the code description.
 * @memberof onion_response_t
 */
const char *onion_response_code_description(int code){
	switch(code){
		case HTTP_OK:
			return "OK";
		case HTTP_CREATED:
			return "CREATED";
		case HTTP_PARTIAL_CONTENT:
			return "PARTIAL CONTENT";
		case HTTP_MULTI_STATUS:
			return "MULTI STATUS";
			
		case HTTP_SWITCH_PROTOCOL:
			return "SWITCHING PROTOCOLS";
			
		case HTTP_MOVED:
			return "MOVED";
		case HTTP_REDIRECT:
			return "REDIRECT";
		case HTTP_SEE_OTHER:
			return "SEE OTHER";
		case HTTP_NOT_MODIFIED:
			return "NOT MODIFIED";
		case HTTP_TEMPORARY_REDIRECT:
			return "TEMPORARY REDIRECT";

		case HTTP_BAD_REQUEST:
			return "BAD REQUEST";
		case HTTP_UNAUTHORIZED:
			return "UNAUTHORIZED";
		case HTTP_FORBIDDEN:
			return "FORBIDDEN";
		case HTTP_NOT_FOUND:
			return "NOT FOUND";
		case HTTP_METHOD_NOT_ALLOWED:
			return "METHOD NOT ALLOWED";

		case HTTP_INTERNAL_ERROR:
			return "INTERNAL ERROR";
		case HTTP_NOT_IMPLEMENTED:
			return "NOT IMPLEMENTED";
		case HTTP_BAD_GATEWAY:
			return "BAD GATEWAY";
		case HTTP_SERVICE_UNAVALIABLE:
			return "SERVICE UNAVALIABLE";
	}
	return "CODE UNKNOWN";
}


/**
 * @short Returns the headers dictionary, so user can add repeated headers
 * 
 * Only simple use case is to add several coockies; using normal set_header is not possible, 
 * but accessing the dictionary user can add repeated headers without problem.
 */
onion_dict *onion_response_get_headers(onion_response *res){
	return res->headers;
}
