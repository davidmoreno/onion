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
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>

#include "server.h"
#include "dict.h"
#include "request.h"
#include "response.h"
#include "handler.h"
#include "server.h"
#include "types_internal.h"
#include "codecs.h"
#include "log.h"

/// Known token types. This is merged with onion_connection_status as return value at token readers.
typedef enum{
	STRING=1001, // I start on 1000, to prevent collission with error codes
	KEY=1002,
	LINE=1003,
	NEW_LINE=1004,
	URLENCODE=1005,
}onion_token_token;


typedef struct onion_token_s{
	char str[256];
	size_t pos;
	
	char *extra; // Only used when need some previous data, like at header value, i need the key
	size_t extra_length;
}onion_token;

typedef struct onion_buffer_s{
	const char *data;
	size_t length;
	off_t pos;
}onion_buffer;

static onion_connection_status onion_request_process(onion_request *req);
static void onion_request_parse_query_to_dict(onion_dict *dict, char *p);
static int onion_request_parse_query(onion_request *req);
static onion_connection_status prepare_POST(onion_request *req);

/// Reads a string until a non-string char. Returns an onion_token
onion_token_token token_read_STRING(onion_token *token, onion_buffer *data){
	if (data->pos>=data->length)
		return OCS_NEED_MORE_DATA;
	
	char c=data->data[data->pos++];
	while (!isspace(c)){
		token->str[token->pos++]=c;
		if (data->pos>=data->length)
			return OCS_NEED_MORE_DATA;
		if (token->pos>=(sizeof(token->str)-1)){
			ONION_ERROR("Token too long to parse it. Part read is %s (%d bytes)",token->str,token->pos);
			return OCS_INTERNAL_ERROR;
		}
		c=data->data[data->pos++];
	}
	token->str[token->pos]='\0';
	//ONION_DEBUG0("Found STRING token %s",token->str);
	return STRING;
}

/// Reads a string until a ':' is found. Returns an onion_token. Also detects an empty line.
onion_token_token token_read_KEY(onion_token *token, onion_buffer *data){
	if (data->pos>=data->length)
		return OCS_NEED_MORE_DATA;
	
	char c=data->data[data->pos++];
	while (c!=':' && c!='\n'){
		token->str[token->pos++]=c;
		if (data->pos>=data->length)
			return OCS_NEED_MORE_DATA;
		if (token->pos>=(sizeof(token->str)-1)){
			ONION_ERROR("Token too long to parse it. Part read is %s (%d bytes)",token->str,token->pos);
			return OCS_INTERNAL_ERROR;
		}
		c=data->data[data->pos++];
	}
	int ret=KEY;
	token->str[token->pos]='\0';
	if (c!=':'){
		if ( (token->pos==1 && token->str[0]=='\r' && c=='\n' ) ||
				 (token->pos==0 && c=='\n' ) )
			ret=NEW_LINE;
		else{
			ONION_ERROR("When parsing header, found a non valid key token: %s",token->str);
			ret=OCS_INTERNAL_ERROR;
		}
	}
	
	//ONION_DEBUG0("Found KEY token %s",token->str);
	return ret;
}

/// Reads a string until a '\n|\r\n' is found. Returns an onion_token.
onion_token_token token_read_LINE(onion_token *token, onion_buffer *data){
	if (data->pos>=data->length)
		return OCS_NEED_MORE_DATA;
	
	char c=data->data[data->pos++];
	while (c!='\n'){
		token->str[token->pos++]=c;
		if (data->pos>=data->length)
			return OCS_NEED_MORE_DATA;
		if (token->pos>=(sizeof(token->str)-1)){
			ONION_ERROR("Token too long to parse it. Part read is %s (%d bytes)",token->str,token->pos);
			return OCS_INTERNAL_ERROR;
		}
		c=data->data[data->pos++];
	}
	if (token->str[token->pos-1]=='\r')
		token->str[token->pos-1]='\0';
	else
		token->str[token->pos]='\0';
	
	//ONION_DEBUG0("Found LINE token %s",token->str);
	return LINE;
}

/// Reads a string until a '\n|\r\n' is found. Returns an onion_token.
onion_token_token token_read_URLENCODE(onion_token *token, onion_buffer *data){
	if (data->pos>=data->length)
		return OCS_NEED_MORE_DATA;
	
	int l=data->length-data->pos;
	if (l > (token->extra_length-token->pos))
		l=token->extra_length-token->pos;
	
	memcpy(&token->extra[token->pos], &data->data[data->pos], l);
	token->pos+=l;
	data->length-=l;
	
	if (data->length == (token->extra_length-token->pos))
		return URLENCODE;
	return OCS_NEED_MORE_DATA;
}

static onion_connection_status parse_POST_urlencode(onion_request *req, onion_buffer *data){
	onion_token *token=req->parser_data;
	onion_token_token res=token_read_URLENCODE(token, data);
	
	if (res<=1000)
		return res;
	
	req->POST=onion_dict_new();
	onion_request_parse_query_to_dict(req->POST, token->extra);

	return onion_request_process(req);
}

static onion_connection_status parse_headers_KEY(onion_request *req, onion_buffer *data);

static onion_connection_status parse_headers_VALUE(onion_request *req, onion_buffer *data){
	onion_token *token=req->parser_data;
	onion_token_token res=token_read_LINE(token, data);
	
	if (res<=1000)
		return res;

	char *p=token->str; // skips leading spaces
	while (isspace(*p)) p++;
	
	onion_dict_add(req->headers,token->extra,p, OD_DUP_VALUE);
	token->extra=NULL;
	
	token->pos=0;
	
	req->parser=parse_headers_KEY;
	
	return OCS_NEED_MORE_DATA; // Get back recursion if any, to prevent too long callstack (on long headers) and stack ovrflow.
}


static onion_connection_status parse_headers_KEY(onion_request *req, onion_buffer *data){
	onion_token *token=req->parser_data;
	onion_token_token res=token_read_KEY(token, data);
	
	if (res<=1000)
		return res;

	if ( res == NEW_LINE ){
		token->pos=0;
		if (!(req->flags&OR_POST))
			return onion_request_process(req);
		
		return prepare_POST(req);
	}
	
	token->extra=strdup(token->str);
	token->pos=0;
	
	req->parser=parse_headers_VALUE;
	
	return parse_headers_VALUE(req, data);
}


static onion_connection_status parse_headers_KEY_skip_NR(onion_request *req, onion_buffer *data){
	char c=data->data[data->pos];
	while(c=='\r' || c=='\n'){
		c=data->data[++data->pos];
		if (data->pos>=data->length)
			return OCS_INTERNAL_ERROR;
	}
	if (!req->GET)
		req->GET=onion_dict_new();
	
	req->parser=parse_headers_KEY;
	
	return parse_headers_KEY(req, data);
}

static onion_connection_status parse_headers_VERSION(onion_request *req, onion_buffer *data){
	onion_token *token=req->parser_data;
	onion_token_token res=token_read_STRING(token, data);
	
	if (res<=1000)
		return res;

	if (strcmp(token->str,"HTTP/1.1")==0)
		req->flags|=OR_HTTP11;
	
	token->pos=0;
	req->parser=parse_headers_KEY_skip_NR;
	
	return parse_headers_KEY_skip_NR(req, data);
}


static onion_connection_status parse_headers_URL(onion_request *req, onion_buffer *data){
	onion_token *token=req->parser_data;
	onion_token_token res=token_read_STRING(token, data);
	
	if (res<=1000)
		return res;

	req->path=req->fullpath=strdup(token->str);
	onion_request_parse_query(req);
	
	token->pos=0;
	req->parser=parse_headers_VERSION;
	
	return parse_headers_VERSION(req, data);
}

static onion_connection_status parse_headers_GET(onion_request *req, onion_buffer *data){
	onion_token *token=req->parser_data;
	onion_token_token res=token_read_STRING(token, data);
	
	if (res<=1000)
		return res;
	
	if (strcmp(token->str,"GET")==0)
		req->flags|=OR_GET;
	else if (strcmp(token->str,"HEAD")==0)
		req->flags|=OR_HEAD;
	else if (strcmp(token->str,"POST")==0)
		req->flags|=OR_POST;
	else{
		ONION_ERROR("Unknown method '%s'",token->str);
		return OCS_NOT_IMPLEMENTED;
	}
	
	token->pos=0;
	req->parser=parse_headers_URL;
	
	return parse_headers_URL(req, data);
}



/**
 * @short Write some data into the request, and passes it line by line to onion_request_fill
 *
 * It features a state machine, from req->parse_state.
 *
 * Depending on the state input is redirected to a diferent parser, one for headers, POST url encoded data... 
 * 
 * @return Returns the number of bytes writen, or <=0 if connection should close, according to onion_connection_status
 * @see onion_connection_status
 */
onion_connection_status onion_request_write(onion_request *req, const char *data, size_t length){
	onion_token *token;
	if (!req->parser_data){
		token=req->parser_data=calloc(1,sizeof(onion_token));
		req->parser=parse_headers_GET;
	}
	
	onion_connection_status (*parse)(onion_request *req, onion_buffer *data);
	parse=req->parser;
	
	if (parse){
		onion_buffer odata={ data, length, 0};
		while (odata.length>odata.pos){
			int r=parse(req, &odata);
			if (r!=OCS_NEED_MORE_DATA){
				if (token->extra){
					free(token->extra);
					token->extra=NULL;
				}
				return r;
			}
			parse=req->parser;
		}
		return OCS_NEED_MORE_DATA;
	}
	
	return OCS_INTERNAL_ERROR;
}

/**
 * @short Parses the query to unquote the path and get the query.
 */
static int onion_request_parse_query(onion_request *req){
	if (!req->fullpath)
		return 0;
	if (req->GET) // already done
		return 1;

	char *p=req->fullpath;
	char have_query=0;
	while(*p){
		if (*p=='?'){
			have_query=1;
			break;
		}
		p++;
	}
	*p='\0';
	onion_unquote_inplace(req->fullpath);
	if (have_query){ // There are querys.
		p++;
		req->GET=onion_dict_new();
		onion_request_parse_query_to_dict(req->GET, p);
	}
	return 1;
}

/**
 * @short Parses the query part to a given dictionary.
 * 
 * The data is overwriten as necessary. It is NOT dupped, so if you free this char *p, please free the tree too.
 */
static void onion_request_parse_query_to_dict(onion_dict *dict, char *p){
	ONION_DEBUG0("Query to dict %s",p);
	char *key, *value;
	int state=0;  // 0 key, 1 value
	key=p;
	while(*p){
		if (state==0){
			if (*p=='='){
				*p='\0';
				value=p+1;
				state=1;
			}
		}
		else{
			if (*p=='&'){
				*p='\0';
				onion_unquote_inplace(key);
				onion_unquote_inplace(value);
				//ONION_DEBUG0("Adding key %s=%-16s",key,value);
				onion_dict_add(dict, key, value, 0);
				key=p+1;
				state=0;
			}
		}
		p++;
	}
	if (state!=0){
		onion_unquote_inplace(key);
		onion_unquote_inplace(value);
		//ONION_DEBUG0("Adding key %s=%-16s",key,value);
		onion_dict_add(dict, key, value, 0);
	}
}

/**
 * @short Processes one request, calling the handler.
 */
static onion_connection_status onion_request_process(onion_request *req){
	if (req->parser_data){
					free(req->parser_data);
					req->parser_data=NULL;
	}
	req->parser=NULL;
	return onion_server_handle_request(req);
}

/**
 * @short Prepares the POST
 */
static onion_connection_status prepare_POST(onion_request *req){
	// ok post
	onion_token *token=req->parser_data;
	const char *content_type=onion_dict_get(req->headers, "Content-Type");
	const char *content_length=onion_dict_get(req->headers, "Content-Length");
	
	if (!content_length){
		ONION_ERROR("I need the content length header to support POST data");
		return OCS_INTERNAL_ERROR;
	}
	size_t cl=atol(content_length);
	
	if (!content_length || strcmp(content_type, "application/x-www-form-urlencoded")==0){
		if (cl>req->server->max_post_size){
			ONION_ERROR("Asked to send much POST data. Failing");
			return OCS_INTERNAL_ERROR;
		}
		token->extra=malloc(cl);
		token->extra_length=cl;
		token->pos=0;
		
		req->parser=parse_POST_urlencode;
		return OCS_NEED_MORE_DATA;
	}
	return OCS_INTERNAL_ERROR;
}
