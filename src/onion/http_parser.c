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

#include "types.h"
#include "types_internal.h"
#include "log.h"
#include "ro_block.h"
#include "codecs.h"
#include "dict.h"
#include "request.h"
#include <string.h>

extern const char *onion_request_methods[16];

/**
 * @short Parses the query part to a given dictionary.
 * 
 * The data is overwriten as necessary. It is NOT dupped, so if you free this char *p, please free the tree too.
 */
static void onion_request_parse_query_to_dict(onion_dict *dict, char *p){
	ONION_DEBUG0("Query to dict %s",p);
	char *key=NULL, *value=NULL;
	int state=0;  // 0 key, 1 value
	key=p;
	while(*p){
		if (state==0){
			if (*p=='='){
				*p='\0';
				value=p+1;
				state=1;
			}
			else if (*p=='&'){
				*p='\0';
				onion_unquote_inplace(key);
				ONION_DEBUG0("Adding key %s",key);
				onion_dict_add(dict, key, "", 0);
				key=p+1;
				state=0;
			}
		}
		else{
			if (*p=='&'){
				*p='\0';
				onion_unquote_inplace(key);
				onion_unquote_inplace(value);
				ONION_DEBUG0("Adding key %s=%-16s",key,value);
				onion_dict_add(dict, key, value, 0);
				key=p+1;
				state=0;
			}
		}
		p++;
	}
	if (state==0){
		if (key[0]!='\0'){
			onion_unquote_inplace(key);
			ONION_DEBUG0("Adding key %s",key);
			onion_dict_add(dict, key, "", 0);
		}
	}
	else{
		onion_unquote_inplace(key);
		onion_unquote_inplace(value);
		ONION_DEBUG0("Adding key %s=%-16s",key,value);
		onion_dict_add(dict, key, value, 0);
	}
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

onion_connection_status onion_http_parse(onion_request *req, onion_ro_block *block){
	ONION_DEBUG("Parse: %s", onion_ro_block_get(block));
	if (onion_ro_block_remaining(block)<7){
		ONION_ERROR("Petition too small to be valid. Not even looking into it");
		return OCS_INTERNAL_ERROR;
	}
	const char *method=onion_ro_block_get_token(block,' ');
	
	ONION_DEBUG("Method %s",method);
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
	
	const char *url=onion_ro_block_get_token(block, ' ');
	ONION_DEBUG("URL is %s", url);
	req->fullpath=strdup(url);
	onion_request_parse_query(req);
	
	const char *http_version=onion_ro_block_get_token(block, '\n');
	ONION_DEBUG("Version is %s", http_version);
	if (strcmp(http_version,"HTTP/1.1")==0)
		req->flags|=OR_HTTP11;

	if (!req->GET)
		req->GET=onion_dict_new();
	
	return OCS_REQUEST_READY;
}
