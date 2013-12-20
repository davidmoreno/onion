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

#define _GNU_SOURCE
#include <string.h>
#include <assert.h>
#include <execinfo.h>
#include <fcntl.h>

#include "types.h"
#include "types_internal.h"
#include "log.h"
#include "parser_block.h"
#include "codecs.h"
#include "dict.h"
#include "request.h"
#include "block.h"

extern const char *onion_request_methods[16];

onion_connection_status onion_http_parse(onion_request *req, onion_parser_block *block);
onion_connection_status onion_http_parse_petition(onion_request *req, char *line);
onion_connection_status onion_http_parse_headers(onion_request *req, char *line);

onion_connection_status onion_http_parse_prepare_for_POST(onion_request *req);
onion_connection_status onion_http_parse_POST_urlencoded(onion_request *req, char *line);
onion_connection_status onion_http_parse_POST_multipart(onion_request *req, char *line);
onion_connection_status onion_http_parse_POST_multipart_header(onion_request *req, char *line);
onion_connection_status onion_http_parse_POST_multipart_data(onion_request *req, char *line);
onion_connection_status onion_http_parse_POST_multipart_file(onion_request *req, onion_parser_block *block);
void onion_http_parse_free(void *data);

typedef struct{
	onion_connection_status (*parser)(onion_request *req, char *line);
	const char *last_key;
	struct{
		char *marker;
		int marker_size;
		char *current_name;
		onion_block *data;
		int fd;
		size_t filesize;
	}multipart;
}http_parser_data;

void onion_http_parser_init(onion_request *req){
	ONION_DEBUG("Init http parser");
	if (req->parser.data && req->parser.free)
		req->parser.free(req->parser.data);
	req->parser.parse=onion_http_parse;
	req->parser.data=calloc(1, sizeof(http_parser_data));
	req->parser.free=onion_http_parse_free;
	http_parser_data *pd=(http_parser_data*)req->parser.data;
	pd->parser=onion_http_parse_petition;
	pd->multipart.data=NULL; //onion_block_new();
}

onion_connection_status onion_http_parse(onion_request *req, onion_parser_block *block){
	char *line;
	onion_connection_status ret=OCS_NEED_MORE_DATA;
	http_parser_data *pd=(http_parser_data*)req->parser.data;
	if (!pd){
		ONION_ERROR("Bad formed http parser data");
		return OCS_INTERNAL_ERROR;
	}
	while ( (ret==OCS_NEED_MORE_DATA) && pd->parser && ( (line=onion_parser_block_get_to_nl(block)) != NULL ) ){
	#ifdef __DEBUG__0
		char **bs=backtrace_symbols((void * const *)&pd->parser, 1);
		ONION_DEBUG0("Line: <%s>, parser <%s>", line, bs[0]);
		free(bs);
	#endif

		ret=pd->parser(req, line);
	}
	
	if (ret == OCS_REQUEST_READY){ // restarting the parser
		ONION_DEBUG0("Process request.");
		pd->parser=onion_http_parse_petition;
	}
	
	ONION_DEBUG0("Return %d (%d is OCS_REQUEST_READY)", ret, OCS_REQUEST_READY);
	return ret;
}

void onion_http_parse_free(void *data){
	http_parser_data *pd=(http_parser_data*)data;
	if (pd->multipart.data)
		onion_block_free(pd->multipart.data);
	free(pd);
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
			return OCS_INTERNAL_ERROR;
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
	
	if (line[0]==' ' || line[0]=='\t'){ // multiline header
		http_parser_data *pd=(http_parser_data*)req->parser.data;
		ONION_DEBUG("Multiline header, last key was %s (%d/%c)", pd->last_key, line[0], line[0]);
		char *nl=onion_str_strip(line);
		if (!*nl) // empty multiline line
			return OCS_NEED_MORE_DATA;
		onion_block *bl=onion_block_new();
		onion_block_add_str(bl, onion_dict_get(req->headers, pd->last_key));
		onion_block_add_char(bl,' ');
		onion_block_add_str(bl, nl);
		char *key=strdup(pd->last_key); // redup it.. a nice game here
		onion_dict_add(req->headers, key, onion_block_data(bl), OD_DUP_VALUE|OD_FREE_KEY|OD_REPLACE);
		onion_block_free(bl);
		pd->last_key=key;
		return OCS_NEED_MORE_DATA;
	}
	
	key = onion_str_strip(onion_str_get_token(&line, ':'));
	
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
		http_parser_data *pd=(http_parser_data*)req->parser.data;
		value=onion_str_strip(value);
		ONION_DEBUG0("Got header: %s=%s", key, value);
		key=strdup(key); // I manually dup the key to use it for last_key
		onion_dict_add(req->headers, key, value, OD_DUP_VALUE|OD_FREE_KEY);
		pd->last_key=key;
	}
	else{
		return OCS_INTERNAL_ERROR;
	}
	
	return OCS_NEED_MORE_DATA;
}

onion_connection_status onion_http_parse_prepare_for_POST(onion_request *req){
	req->POST=onion_dict_new();
	
	http_parser_data *pd=(http_parser_data*)req->parser.data;
	const char *content_type=onion_request_get_header(req, "Content-Type");
	if (content_type && strncmp(content_type,"multipart/form-data",sizeof("multipart/form-data")-1)==0){
		pd->parser=onion_http_parse_POST_multipart;
		
		// All this because of no strdupa
		int len=strlen(content_type);
		char cttt[len];
		memcpy(cttt,content_type,len);
		char *ctt=(char*)cttt;
		
		pd->multipart.marker=NULL;
		while(*ctt){
			char c;
			char *boundary=onion_str_get_token2(&ctt, ";=", &c);
			if (!boundary)
				return OCS_INTERNAL_ERROR;
			boundary=onion_str_strip(boundary);
// 			ONION_DEBUG0("Got content_type part <%s>, <%c> %d %d", boundary, c,
// 				c=='=', strcasecmp(boundary,"boundary")==0);
			if (c=='=' && strcasecmp(boundary,"boundary")==0){
				boundary=onion_str_get_token(&ctt, ';');
				ONION_DEBUG0("Boundary marker is <%s>",boundary);
				if (!boundary)
					return OCS_INTERNAL_ERROR;
				pd->multipart.marker_size=strlen(boundary)+2;
				pd->multipart.marker=malloc(pd->multipart.marker_size+3);
				pd->multipart.marker[0]=pd->multipart.marker[1]='-';
				memcpy(pd->multipart.marker+2, boundary, pd->multipart.marker_size-1);
				ONION_DEBUG0("Boundary marker is <%s> <%s>",boundary, pd->multipart.marker);
				return OCS_NEED_MORE_DATA;
			}
		}
		return OCS_INTERNAL_ERROR;
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
	ONION_DEBUG0("Checking first marker: <%s> <%.*s> %d", line, pd->multipart.marker_size, pd->multipart.marker,
		strncmp(line, pd->multipart.marker, pd->multipart.marker_size)==0
	);
	if (strncmp(line, pd->multipart.marker, pd->multipart.marker_size)==0){
		if (line[pd->multipart.marker_size]=='-' && 
			line[pd->multipart.marker_size+1]=='-'){
			return OCS_REQUEST_READY;
		}
		pd->parser=onion_http_parse_POST_multipart_header;
		return OCS_NEED_MORE_DATA;
	}
	return OCS_INTERNAL_ERROR;
}

onion_connection_status onion_http_parse_POST_multipart_header(onion_request *req, char *line){
	http_parser_data *pd=(http_parser_data*)req->parser.data;
	if (!*line){ // No more headers
		ONION_DEBUG0("Process multipart header for <%s>", pd->multipart.current_name);
		if (pd->multipart.fd>=0){
			pd->parser=NULL;
			req->parser.parse=onion_http_parse_POST_multipart_file;
		}
		else
			pd->parser=onion_http_parse_POST_multipart_data;
		return OCS_NEED_MORE_DATA;
	}
	char *key=onion_str_get_token(&line,':');
	
	ONION_DEBUG0("multipart header: %s", line);
	ONION_DEBUG0("Content disposition <%s> %d", key, strcasecmp(key,"content-disposition")==0);
	if (key && strcasecmp(key,"content-disposition")==0){
		char c;
		while ( (key=onion_str_get_token2(&line, ";=", &c)) ){
			key=onion_str_strip(key);
			ONION_DEBUG0("subkey <%s>", key);
			if (c=='='){
				if (strcasecmp(key, "name")==0){
					char *v=onion_str_unquote(onion_str_get_token2(&line, ";=", &c));
					if (pd->multipart.current_name)
						free(pd->multipart.current_name);
					pd->multipart.current_name=strdup(v);
					ONION_DEBUG("Current name is <%s>", pd->multipart.current_name);
				}
				if (strcasecmp(key, "filename")==0){
					char *v=onion_str_unquote(onion_str_get_token2(&line, ";=", &c));
					if (pd->multipart.current_name)
						onion_dict_add(req->POST, pd->multipart.current_name,
													v, OD_DUP_ALL);
					else{
						ONION_WARNING("Error in post request, no name for filename field, not adding filename to POST dict");
					}
					ONION_DEBUG("Current filename is <%s>", v);
					if (!req->FILES)
						req->FILES=onion_dict_new();
					
					char tmpfilename[]="/tmp/onion-XXXXXX";
					if (mkstemp(tmpfilename)<0){
						ONION_WARNING("Could not create temporal file");
						return OCS_INTERNAL_ERROR;
					}
					pd->multipart.fd=open(tmpfilename, O_CREAT|O_WRONLY, 0600);
					if (pd->multipart.fd<0){
						ONION_WARNING("Could not create temporal file");
						return OCS_INTERNAL_ERROR;
					}
					
					ONION_DEBUG("Load a file");
				}
			}
		}
	}
	
	return OCS_NEED_MORE_DATA;
}

onion_connection_status onion_http_parse_POST_multipart_data(onion_request *req, char *line){
	http_parser_data *pd=(http_parser_data*)req->parser.data;
	ONION_DEBUG0("Data <%s>", line);
	char *eop=strstr(line, pd->multipart.marker);
	if (eop){
		// maybe add some data before marker
		onion_dict_add(req->POST, 
									 pd->multipart.current_name, 
									 onion_block_data(pd->multipart.data),
									 OD_DUP_ALL);
		free(pd->multipart.current_name);
		pd->multipart.current_name=NULL;
		onion_block_clear(pd->multipart.data);
		
		if (eop[pd->multipart.marker_size]=='-' && 
			eop[pd->multipart.marker_size+1]=='-'){
			return OCS_REQUEST_READY;
		}
		else{
			ONION_DEBUG0("Ok, part done, next headers");
			pd->parser=onion_http_parse_POST_multipart_header;
		}
	}
	else{
		onion_block_add_str(pd->multipart.data,line);
	}
	return OCS_NEED_MORE_DATA;
}

onion_connection_status onion_http_parse_POST_multipart_file(onion_request *req, onion_parser_block *block){
	http_parser_data *pd=(http_parser_data*)req->parser.data;
	//ONION_DEBUG0("Data <%s>", line);
	char *data=onion_parser_block_get(block);
	size_t data_len=onion_parser_block_remaining(block);
	char *eop=memmem(data, data_len,
									 pd->multipart.marker, pd->multipart.marker_size);
	if (eop){
		data_len=eop-data;
	}
	ssize_t ret=write(pd->multipart.fd, data, data_len);
	onion_parser_block_advance(block, data_len);
	pd->multipart.filesize+=ret;
	ONION_DEBUG0("Wrote %ld bytes, total %ld", ret, pd->multipart.filesize);
	if (ret!=data_len)
		return OCS_INTERNAL_ERROR;
	if (eop){
		ONION_DEBUG0("Done");
		close(pd->multipart.fd);
		pd->multipart.fd=-1;
		
		onion_parser_block_advance(block, pd->multipart.marker_size);
		
		req->parser.parse=onion_http_parse;
		//pd->parser_block=NULL;
		pd->parser=onion_http_parse_POST_multipart_header;
		
		if (eop[pd->multipart.marker_size]=='-' &&
			eop[pd->multipart.marker_size+1]=='-'){
			onion_parser_block_get_to_nl(block);
			return OCS_REQUEST_READY;
		}
	}
	return OCS_NEED_MORE_DATA;
}
