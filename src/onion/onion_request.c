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
#include <stdlib.h>
#include <libgen.h>

#include "onion_server.h"
#include "onion_dict.h"
#include "onion_request.h"
#include "onion_response.h"
#include "onion_handler.h"
#include "onion_server.h"
#include "onion_types_internal.h"
#include "onion_codecs.h"
#include "onion_log.h"


static int onion_request_parse_query(onion_request *req);

// Internally it uses req->parse_state, states are:
typedef enum parse_state_e{
	CLEAN=0,
	HEADERS=1,
	POST_DATA=2,
	FINISHED=3,
}parse_state;



/**
 *  @short Creates a request object
 * 
 * @param server onion_server that will be used for writing and some other data
 * @param socket Socket as needed by onion_server write method.
 * @param client_info String that describes the client, for example, the IP address.
 */
onion_request *onion_request_new(onion_server *server, void *socket, const char *client_info){
	onion_request *req;
	req=malloc(sizeof(onion_request));
	memset(req,0,sizeof(onion_request));
	
	req->server=server;
	req->headers=onion_dict_new();
	req->socket=socket;
	req->parse_state=0;
	req->buffer_pos=0;
	req->files=NULL;
	req->post=NULL;
	if (client_info) // This is kept even on clean
		req->client_info=strdup(client_info);
	else
		req->client_info=NULL;

	return req;
}

/// Deletes a request and all its data
void onion_request_free(onion_request *req){
	onion_dict_free(req->headers);
	
	if (req->fullpath)
		free(req->fullpath);
	if (req->query)
		onion_dict_free(req->query);
	if (req->post)
		onion_dict_free(req->post);
	if (req->files)
		onion_dict_free(req->files);

	if (req->client_info)
		free(req->client_info);
	
	free(req);
}

/// Parses the query part to a given dictionary.
static void onion_request_parse_query_to_dict(onion_dict *dict, const char *p){
	char key[32], value[256];
	int state=0;  // 0 key, 1 value
	int i=0;
	while(*p){
		if (state==0){
			if (*p=='='){
				key[i]='\0';
				state=1;
				i=-1;
			}
			else
				key[i]=*p;
		}
		else{
			if (*p=='&'){
				value[i]='\0';
				onion_unquote_inplace(key);
				onion_unquote_inplace(value);
				onion_dict_add(dict, key, value, OD_DUP_ALL);
				state=0;
				i=-1;
			}
			else
				value[i]=*p;
		}
		p++;
		i++;
	}
	if (i!=0 || state!=0){
		if (state==0){
			key[i]='\0';
			value[0]='\0';
		}
		else
			value[i]='\0';
		onion_unquote_inplace(key);
		onion_unquote_inplace(value);
		onion_dict_add(dict, key, value, OD_DUP_ALL);
	}
}

/// Parses the first query line, GET / HTTP/1.1
static onion_connection_status onion_request_fill_query(onion_request *req, const char *data){
	ONION_DEBUG("Request: %s",data);
	char method[16], url[256], version[16];
	sscanf(data,"%15s %255s %15s",method, url, version);
	
	if (strcmp(method,"GET")==0)
		req->flags|=OR_GET;
	else if (strcmp(method,"POST")==0)
		req->flags|=OR_POST;
	else if (strcmp(method,"HEAD")==0)
		req->flags|=OR_HEAD;
	else
		return OCS_NOT_IMPLEMENTED; // Not valid method detected.

	if (strcmp(version,"HTTP/1.1")==0)
		req->flags|=OR_HTTP11;

	req->path=strndup(url,sizeof(url));
	req->fullpath=req->path;
	onion_request_parse_query(req); // maybe it consumes some CPU and not always needed.. but I need the unquotation.
	
	return OCS_NEED_MORE_DATA;
}

/// Reads a header and sets it at the request
static onion_connection_status onion_request_fill_header(onion_request *req, const char *data){
	char header[32], value[256];
	sscanf(data, "%31s", header);
	int i=0; 
	const char *p=&data[strlen(header)+1];
	while(*p){
		value[i++]=*p++;
		if (i==sizeof(value)-1){
			break;
		}
	}
	value[i]=0;
	header[strlen(header)-1]='\0'; // removes the :
	onion_dict_add(req->headers, header, value, OD_DUP_ALL);
	return OCS_NEED_MORE_DATA;
}

/**
 * @short Processes one request, calling the handler.
 */
static onion_connection_status onion_request_process(onion_request *req){
	req->parse_state=FINISHED;
	int s=onion_server_handle_request(req);
	return s;
}

/**
 * @short Partially fills a request. One line each time.
 * 
 * @returns onion_request_write_status that sets if there is internal error, keep alive...
 * @see onion_request_write_status
 */
onion_connection_status onion_request_fill(onion_request *req, const char *data){
	ONION_DEBUG0("Request (%d): %s",req->parse_state, data);

	switch(req->parse_state){
	case CLEAN:
		req->parse_state=HEADERS;
		return onion_request_fill_query(req, data);
	case HEADERS:
		if (data[0]=='\0'){
			if (req->flags&OR_POST){
				req->parse_state=POST_DATA;
				return OCS_NEED_MORE_DATA;
			}
			else{
				return onion_request_process(req);
			}
		}
		return onion_request_fill_header(req, data);
	case FINISHED:
		ONION_ERROR("Not accepting more data on this status. Clean the request if you want to start a new one.");
	}
	return OCS_INTERNAL_ERROR;
}

/**
 * @short Parses the query to unquote the path and get the query.
 */
static int onion_request_parse_query(onion_request *req){
	if (!req->path)
		return 0;
	if (req->query) // already done
		return 1;
	
	char cleanurl[256];
	int i=0;
	char *p=req->path;
	while(*p){
		if (*p=='?')
			break;
		cleanurl[i++]=*p;
		p++;
	}
	cleanurl[i++]='\0';
	onion_unquote_inplace(cleanurl);
	if (*p){ // There are querys.
		p++;
		req->query=onion_dict_new();
		onion_request_parse_query_to_dict(req->query, p);
	}
	free(req->fullpath);
	req->fullpath=strndup(cleanurl, sizeof(cleanurl));
	req->path=req->fullpath;
	return 1;
}

/// Fills the post data.
static onion_connection_status onion_request_write_post(onion_request *req, const char *data, size_t length){
	size_t content_length=0;
	const char *cl=onion_dict_get(req->headers,"Content-Length");
	if (!cl){
		ONION_ERROR("Need Content-Length header when in POST method. Aborting petition.");
		return OCS_INTERNAL_ERROR;
	}
	content_length=atoi(cl);
	if (sizeof(req->buffer)<content_length){
		ONION_WARNING("Onion not yet prepared for POST with more than %ld bytes of data (this have %ld)",(unsigned int)sizeof(req->buffer),(unsigned int)content_length);
		return OCS_INTERNAL_ERROR;
	}
	
	size_t l=(length<content_length) ? length : content_length;
	ONION_DEBUG("Expecting a POST of %d bytes, got %d, max %d",(int)content_length,(int)l,(int)sizeof(req->buffer));
	memcpy(&req->buffer[req->buffer_pos], data, l);
	req->buffer[l]=0;

	ONION_DEBUG("POST data %s",req->buffer);
	req->post=onion_dict_new();
	onion_request_parse_query_to_dict(req->post, req->buffer);
	req->buffer_pos=0;

	int r=onion_request_process(req);
	if (l<length){
		ONION_WARNING("Received more data after POST data. Not expected. Clossing connection");
		return OCS_CLOSE_CONNECTION;
	}
	return r;
}


/**
 * @short Write some data into the request, and passes it line by line to onion_request_fill
 *
 * Just reads line by line and passes the data to onion_request_fill. If on POST state, calls onion_request_write_post. 
 * 
 * @return Returns the number of bytes writen, or <=0 if connection should close, acording to onion_request_write_status_e.
 * @see onion_request_write_status_e onion_request_write_post
 */
onion_connection_status onion_request_write(onion_request *req, const char *data, size_t length){
	size_t i;
	char msgshown=0;
	if (req->parse_state==POST_DATA){
		return onion_request_write_post(req, data, length);
	}
	for (i=0;i<length;i++){
		char c=data[i];
		if (c=='\r') // Just skip it
			continue;
		if (c=='\n'){
			req->buffer[req->buffer_pos]='\0';
			int r=onion_request_fill(req,req->buffer);
			req->buffer_pos=0;
			if (r<=0) // Close connection. Might be a rightfull close, or because of an error. Close anyway.
				return r;
			if (req->parse_state==POST_DATA)
				return onion_request_write_post(req, &data[i+1], length-(i+1));
		}
		else{
			req->buffer[req->buffer_pos]=c;
			req->buffer_pos++;
			if (req->buffer_pos>=sizeof(req->buffer)){ // Overflow on line
				req->buffer_pos--;
				if (!msgshown){
					ONION_ERROR("Read data too long for me (max data length %d chars). Ignoring from that byte on to the end of this line. (%16s...)",(int) sizeof(req->buffer),req->buffer);
					ONION_ERROR("Increase it at onion_request.h and recompile onion.");
					msgshown=1;
				}
			}
		}
	}
	return OCS_NEED_MORE_DATA;
}


/// Returns a pointer to the string with the current path. Its a const and should not be trusted for long time.
const char *onion_request_get_path(onion_request *req){
	return req->path;
}

/// Moves the pointer inside fullpath to this new position, relative to current path.
void onion_request_advance_path(onion_request *req, int addtopos){
	req->path=&req->path[addtopos];
}

/// Gets a header data
const char *onion_request_get_header(onion_request *req, const char *header){
	return onion_dict_get(req->headers, header);
}

/// Gets a query data
const char *onion_request_get_query(onion_request *req, const char *query){
	if (req->query)
		return onion_dict_get(req->query, query);
	return NULL;
}

/// Gets a post data
const char *onion_request_get_post(onion_request *req, const char *query){
	if (req->post)
		return onion_dict_get(req->post, query);
	return NULL;
}

/**
 * @short Cleans a request object to reuse it.
 */
void onion_request_clean(onion_request* req){
	onion_dict_free(req->headers);
	req->headers=onion_dict_new();
	req->parse_state=0;
	req->flags&=0xFF00;
	if (req->fullpath){
		free(req->fullpath);
		req->path=req->fullpath=NULL;
	}
	if (req->query){
		onion_dict_free(req->query);
		req->query=NULL;
	}
}

/**
 * @short Forces the request to process only one request, not doing the keep alive.
 * 
 * This is usefull on non threaded modes, as the keep alive blocks the loop.
 */
void onion_request_set_no_keep_alive(onion_request *req){
	req->flags|=OR_NO_KEEP_ALIVE;
	ONION_DEBUG("Disabling keep alive %X",req->flags);
}

/**
 * @short Returns if current request wants to keep alive.
 * 
 * It is a complex set of circumstances: HTTP/1.1 and no connection: close, or HTTP/1.0 and connection: keep-alive
 * and no explicit set that no keep alive.
 */
int onion_request_keep_alive(onion_request *req){
	if (req->flags&OR_NO_KEEP_ALIVE)
		return 0;
	if (req->flags&OR_HTTP11){
		const char *connection=onion_request_get_header(req,"Connection");
		if (!connection || strcasecmp(connection,"Close")!=0) // Other side wants keep alive
			return 1;
	}
	else{ // HTTP/1.0
		const char *connection=onion_request_get_header(req,"Connection");
		if (connection && strcasecmp(connection,"Keep-Alive")==0) // Other side wants keep alive
			return 1;
	}
	return 0;
}
