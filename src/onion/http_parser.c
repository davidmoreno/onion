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

#include <string.h>
#include <assert.h>

#include "types.h"
#include "types_internal.h"
#include "log.h"
#include "ro_block.h"
#include "codecs.h"
#include "dict.h"
#include "request.h"

extern const char *onion_request_methods[16];

onion_connection_status onion_http_parse_petition(onion_request *req, char *line);
onion_connection_status onion_http_parse_headers(onion_request *req, char *line);

onion_connection_status onion_http_parse_prepare_for_POST(onion_request *req);
onion_connection_status onion_http_parse_POST_urlencoded(onion_request *req, char *line);
onion_connection_status onion_http_parse_POST_multipart(onion_request *req, char *line);
onion_connection_status onion_http_parse_POST_multipart_header(onion_request *req, char *line);
onion_connection_status onion_http_parse_POST_multipart_data(onion_request *req, char *line);

typedef struct{
	onion_connection_status (*parser)(onion_request *req, char *line);
	const char *multipart;
	int multipart_size;
}http_parser_data;


onion_connection_status onion_http_parse(onion_request *req, onion_ro_block *block){
	if (!req->parser.data){
		req->parser.data=malloc(sizeof(http_parser_data));
		http_parser_data *pd=(http_parser_data*)req->parser.data;
		pd->parser=onion_http_parse_petition;
		req->parser.free=free;
	}
	char *line;
	onion_connection_status ret=OCS_NEED_MORE_DATA;
	http_parser_data *pd=(http_parser_data*)req->parser.data;
	while ( (ret==OCS_NEED_MORE_DATA) && ( (line=onion_ro_block_get_to_nl(block)) != NULL ) ){
		ret=pd->parser(req, line);
		//ONION_DEBUG0("Line: <%s>, response %d", line, ret);
	}
	
	if (ret == OCS_REQUEST_READY){ // restarting the parser
		ONION_DEBUG0("Process request.");
		pd->parser=onion_http_parse_petition;
	}
	
	ONION_DEBUG0("Return %d (%d is OCS_REQUEST_READY)", ret, OCS_REQUEST_READY);
	return ret;
}

onion_connection_status onion_http_parse_petition(onion_request *req, char *line){
	const char *method=onion_str_get_token(&line,' ');
	if (!method)
		return OCS_INTERNAL_ERROR;
	
	ONION_DEBUG0("Method %s",method);
	int i;
	for (i=0;i<16;i++){
		if (!onion_request_methods[i]){
			ONION_ERROR("Unknown method '%s' (%d known methods)",method, i);
			return OCS_NOT_IMPLEMENTED;
		}
		if (strcmp(onion_request_methods[i], method)==0){
			ONION_DEBUG0("Method is %s", method);
			req->flags=(req->flags&~0x0F)+i;
			break;
		}
	}
	
	const char *url=onion_str_get_token(&line, ' ');

	ONION_DEBUG0("URL is %s", url);
	req->fullpath=strdup(url);
	onion_request_parse_query(req);

	http_parser_data *pd=(http_parser_data*)req->parser.data;
	if (!*line){
		ONION_DEBUG0("Request without HTTP version");
		pd->parser=onion_http_parse_headers;
		return OCS_NEED_MORE_DATA;
	}
	const char *http_version=line;
	ONION_DEBUG0("HTTP Version is %s", http_version);
	if (http_version && strcmp(http_version,"HTTP/1.1")==0)
		req->flags|=OR_HTTP11;
	else
		ONION_DEBUG0("http version <%s>",http_version);

	
	pd->parser=onion_http_parse_headers;
	return OCS_NEED_MORE_DATA;
}

onion_connection_status onion_http_parse_headers(onion_request *req, char *line){
	//ONION_DEBUG0("Check line <%s>", line);
	char *key, *value;
	
	key = onion_str_get_token(&line, ':');
	
	// End of headers
	if (!key){ 
		http_parser_data *pd=(http_parser_data*)req->parser.data;
		if ( (req->flags & OR_METHODS) == OR_POST ){
			return onion_http_parse_prepare_for_POST(req);
		}
		else
			pd->parser=NULL;

		ONION_DEBUG0("Done");
		return OCS_REQUEST_READY;
	}
	
	// Another header
	value=line;
	if (key && value){
		value=onion_str_strip(value);
		ONION_DEBUG0("Got header: %s=%s", key, value);
		onion_dict_add(req->headers, key, value, 0);
	}
	
	return OCS_NEED_MORE_DATA;
}

onion_connection_status onion_http_parse_prepare_for_POST(onion_request *req){
	http_parser_data *pd=(http_parser_data*)req->parser.data;
	const char *content_type_c=onion_request_get_header(req, "Content-Type");
	ONION_DEBUG0("Prepare for post, content type is <%s>", content_type_c);
	if (content_type_c && strncmp(content_type_c,"multipart/form-data",sizeof("multipart/form-data")-1)==0){
		size_t content_type_l=strlen(content_type_c);
		char *content_type=alloca(content_type_l);
		memcpy(content_type, content_type_c, content_type_l);
		pd->parser=onion_http_parse_POST_multipart;
		char c;
		char *boundary;
		while ( (boundary=onion_str_strip(onion_str_get_token2(&content_type, ";=", &c))) ){
			ONION_DEBUG0("Got <%s> <%c>", boundary, c);
			if (c==';' && boundary && strcasecmp(boundary,"boundary")){
				boundary=onion_str_strip(onion_str_get_token(&content_type, ';'));
				if (boundary){
					ONION_DEBUG0("Set boundary <%s>", boundary);
					pd->multipart=strdup(boundary); // May give problems for not properly getting token. We will see.
				}
				if (!pd->multipart)
					return OCS_INTERNAL_ERROR;
				pd->multipart_size=strlen(pd->multipart);
			}
		}
		assert(pd->multipart != NULL);
		
	}
	else
		pd->parser=onion_http_parse_POST_urlencoded;

	ONION_DEBUG0("Next is POST data");
	return OCS_NEED_MORE_DATA;
}

onion_connection_status onion_http_parse_POST_urlencoded(onion_request *req, char *line){
	ONION_DEBUG0("Process url encoded post: %s", line);
	if (!req->POST)
		req->POST=onion_dict_new();
	onion_request_parse_query_to_dict(req->POST, line);
	return OCS_REQUEST_READY;
}

onion_connection_status onion_http_parse_POST_multipart(onion_request *req, char *line){
	http_parser_data *pd=(http_parser_data*)req->parser.data;
	if (strcmp(line, pd->multipart)){
		pd->parser=onion_http_parse_POST_multipart_header;
		return OCS_NEED_MORE_DATA;
	}
	return OCS_INTERNAL_ERROR;
}

onion_connection_status onion_http_parse_POST_multipart_header(onion_request *req, char *line){
	http_parser_data *pd=(http_parser_data*)req->parser.data;
	if (*line){ // No more headers
		pd->parser=onion_http_parse_POST_multipart_data;
		return OCS_NEED_MORE_DATA;
	}
	ONION_DEBUG("multipart header: %s", line);
	return OCS_NEED_MORE_DATA;
}

onion_connection_status onion_http_parse_POST_multipart_data(onion_request *req, char *line){
	http_parser_data *pd=(http_parser_data*)req->parser.data;
	if (line[0]=='-' && line[1]=='-' && strncmp(line+2, pd->multipart, pd->multipart_size)){
		if (line[pd->multipart_size+2]=='\0'){
			pd->parser=onion_http_parse_POST_multipart_header;
			return OCS_NEED_MORE_DATA;
		}
		if (line[pd->multipart_size+2]=='-' && line[pd->multipart_size+3]=='-' && line[pd->multipart_size+4]=='\0' ){
			pd->parser=NULL;
			return OCS_REQUEST_READY;
		}
	}
	ONION_DEBUG0("Data <%s>", line);
	return OCS_NEED_MORE_DATA;
}
