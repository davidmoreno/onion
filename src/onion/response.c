/*
	Onion HTTP server library
	Copyright (C) 2010 David Moreno Montero

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 3.0 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not see <http://www.gnu.org/licenses/>.
	*/

#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>

#include "dict.h"
#include "request.h"
#include "server.h"
#include "response.h"
#include "types_internal.h"
#include "log.h"

const char *onion_response_code_description(int code);
static int onion_response_write_buffer(onion_response *res);

/**
 * @short Generates a new response object
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
	if (req){
		res->write=req->server->write;
		res->socket=req->socket;
	}
	
	// Sorry for the publicity.
	onion_dict_add(res->headers, "Server", "libonion v0.3 - coralbits.com", 0);
	onion_dict_add(res->headers, "Content-Type", "text/html", 0); // Maybe not the best guess, but really useful.
	//time_t t=time(NULL);
	//onion_dict_add(res->headers, "Date", asctime(localtime(&t)), OD_DUP_VALUE);
	
	return res;
}

/**
 * @short Frees the memory consumed by this object
 * 
 * This function returns the close status: OR_KEEP_ALIVE or OR_CLOSE_CONNECTION as needed.
 * 
 * @returns Whether the connection should be closed or not, or an error status to be handled by server.
 * @see onion_connection_status
 */
onion_connection_status onion_response_free(onion_response *res){
	// write pending data.
	onion_response_write_buffer(res);
	
	if (res->flags&OR_CHUNKED){ // Set the chunked data end.
		void *fd=res->socket;
		onion_write write=res->write;
		write(fd, "0\r\n\r\n",7);
	}
	
	int r=OCS_CLOSE_CONNECTION;
	
	// it is a rare ocassion that there is no request, but although unlikely, it may happend
	if (res->request){
		// keep alive only on HTTP/1.1.
		//ONION_DEBUG("keep alive [req wants] %d && ([skip] %d || [lenght ok] %d || [chunked] %d)", 
		//						onion_request_keep_alive(res->request),
		//						res->flags&OR_SKIP_CONTENT,res->length==res->sent_bytes, res->flags&OR_CHUNKED);
		if ( onion_request_keep_alive(res->request) && 
				 ( res->flags&OR_SKIP_CONTENT || res->length==res->sent_bytes || res->flags&OR_CHUNKED ) 
			 )
			r=OCS_KEEP_ALIVE;
		
		// FIXME! This is no proper logging at all. Maybe use a handler.
		ONION_INFO("[%s] \"%s %s\" %d %d (%s)", res->request->client_info,
							(res->request->flags&OR_GET) ? "GET" : 
							(res->request->flags&OR_HEAD) ? "HEAD" : 
							(res->request->flags&OR_POST) ? "POST" : "UNKNOWN_METHOD",
						res->request->fullpath, res->code, res->sent_bytes,
						(r==OCS_KEEP_ALIVE) ? "Keep-Alive" : "Close connection");
	}
	
	onion_dict_free(res->headers);
	free(res);
	
	return r;
}

/// Adds a header to the response object
void onion_response_set_header(onion_response *res, const char *key, const char *value){
	onion_dict_add(res->headers, key, value, OD_DUP_ALL|OD_REPLACE); // DUP_ALL not so nice on memory side...
}

/// Sets the header length. Normally it should be through set_header, but as its very common and needs some procesing here is a shortcut
void onion_response_set_length(onion_response *res, size_t len){
	char tmp[16];
	sprintf(tmp,"%lu",(unsigned long)len);
	onion_response_set_header(res, "Content-Length", tmp);
	res->length=len;
	res->flags|=OR_LENGTH_SET;
}

/// Sets the return code
void onion_response_set_code(onion_response *res, int  code){
	res->code=code;
}

/// Helper that is called on each header, and writes the header
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

/**
 * @short Writes all the header to the given response
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
	
	if (!(res->flags&OR_LENGTH_SET) && !chunked)
		onion_response_write(res, CONNECTION_CLOSE, sizeof(CONNECTION_CLOSE)-1);
	
	onion_dict_preorder(res->headers, write_header, res);
	
	if (res->request->session_id) // I have session, tell user
		onion_response_printf(res, "Set-Cookie: sessionid=%s; httponly\n", res->request->session_id);
	
	onion_response_write(res,"\r\n",2);
	
	res->sent_bytes=0; // the header size is not counted here.
	
	if (res->request->flags&OR_HEAD){
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
 * 
 * This is the main write data function. If the headers have not been sent yet, they are now.
 * 
 * It's internally used also by the write0 and printf versions.
 * 
 * Also it does some buffering, so data is not sent as written by code, but only in chunks. 
 * These chunks are when the response is finished, or when the internal buffer is full. This
 * helps performance, and eases the programming on the user side.
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
	if (res->buffer_pos==0)
		return 0;
	void *fd=res->request->socket;
	onion_write write=res->write;
	ssize_t w;
	off_t pos=0;
	//ONION_DEBUG0("Write %d bytes",res->buffer_pos);
	if (res->flags&OR_CHUNKED){
		char tmp[16];
		snprintf(tmp,sizeof(tmp),"%X\r\n",(unsigned int)res->buffer_pos);
		if (write(fd, tmp, strlen(tmp))<=0){
			ONION_WARNING("Error writing chunk encoding length. Aborting write.");
			return OCS_CLOSE_CONNECTION;
		}
	}
	while ( (w=write(fd, &res->buffer[pos], res->buffer_pos)) != res->buffer_pos){
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
		write(fd,"\r\n",2);
	}
	res->buffer_pos=0;
	return 0;
}

/// Writes a 0-ended string to the response.
ssize_t onion_response_write0(onion_response *res, const char *data){
	return onion_response_write(res, data, strlen(data));
}

/// Writes some data to the response. Using sprintf format strings. Max final string size: 1024
ssize_t onion_response_printf(onion_response *res, const char *fmt, ...){
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
	return response->write;
}

/// Returns the socket object.
void *onion_response_get_socket(onion_response *response){
	return response->socket;
}


/**
 * @short Returns a const char * string with the code description.
 */
const char *onion_response_code_description(int code){
	switch(code){
		case HTTP_OK:
			return "OK";
		case HTTP_CREATED:
			return "CREATED";
		case HTTP_PARTIAL_CONTENT:
			return "PARTIAL CONTENT";
			
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
	return "INTERNAL ERROR - CODE UNKNOWN";
}

/**
 * @short Sets the writer to use on this response
 * 
 * Normally this is automatically get from the origin request object. Anyway 
 * it exists the option to overwrite it, for example to have a gzip layer.
 */
void onion_response_set_writer(onion_response *res, onion_write write, void *socket){
	res->write=write;
	res->socket=socket;
}
