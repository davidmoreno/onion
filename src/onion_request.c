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

#include "onion_request.h"

/// Creates a request
onion_request *onion_request_new(onion_server *server, void *socket){
	onion_request *req;
	req=malloc(sizeof(onion_request));
	memset(req,0,sizeof(onion_request));
	
	req->server=server;
	req->headers=onion_dict_new();
	req->socket=socket;
	
	return req;
}

/// Deletes a request and all its data
void onion_request_free(onion_request *req){
	onion_dict_free(req->headers);
	
	if (req->fullpath)
		free(req->fullpath);
	if (req->query)
		onion_dict_free(req->query);
	
	free(req);
}

/// Partially fills a request. One line each time.
int onion_request_fill(onion_request *req, const char *data){
	if (!req->path){
		char method[16], url[256], version[16];
		sscanf(data,"%15s %255s %15s",method, url, version);

		if (strcmp(method,"GET")==0)
			req->flags=OR_GET;
		else if (strcmp(method,"POST")==0)
			req->flags=OR_POST;
		else if (strcmp(method,"HEAD")==0)
			req->flags=OR_HEAD;
		else
			return 0; // Not valid method detected.

		if (strcmp(version,"HTTP/1.1")==0)
			req->flags|=OR_HTTP11;
		else
			return 0;

		req->path=strndup(url,sizeof(url));
		req->fullpath=req->path;
	}
	else{
		char header[32], value[256];
		sscanf(data, "%31s", header);
		int i=0; 
		const char *p=&data[strlen(header)+1];
		while(*p && *p!='\n'){
			value[i++]=*p++;
			if (i==sizeof(value)-1){
				break;
			}
		}
		value[i]=0;
		header[strlen(header)-1]='\0'; // removes the :
		onion_dict_add(req->headers, header, value, OD_DUP_ALL);
	}
	return 1;
}

/**
 * @short Performs unquote inplace.
 *
 * It can be inplace as char position is always at least in the same on the destination than in the origin
 */
void onion_request_unquote(char *str){
	char *r=str;
	char *w=str;
	char tmp[3]={0,0,0};
	while (*r){
		if (*r == '%'){
			r++;
			tmp[0]=*r++;
			tmp[1]=*r;
			*w=strtol(tmp, (char **)NULL, 16);
		}
		else if (*r=='+'){
			*w=' ';
		}
		else{
			*w=*r;
		}
		r++;
		w++;
	}
	*w='\0';
}

/// Parses a query string.
int onion_request_parse_query(onion_request *req){
	if (!req->path)
		return 0;
	if (req->query) // already done
		return 1;
	
	char key[32], value[256];
	char cleanurl[256];
	int i=0;
	char *p=req->path;
	while(*p){
		//fprintf(stderr,"%d %c", *p, *p);
		if (*p=='?')
			break;
		cleanurl[i++]=*p;
		p++;
	}
	cleanurl[i++]='\0';
	onion_request_unquote(cleanurl);
	if (*p){ // There are querys.
		p++;
		req->query=onion_dict_new();
		int state=0;
		i=0;
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
					onion_request_unquote(key);
					onion_request_unquote(value);
					onion_dict_add(req->query, key, value, OD_DUP_ALL);
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
			if (state==0)
				key[i]='\0';
			else
				value[i]='\0';
			onion_request_unquote(key);
			onion_request_unquote(value);
			onion_dict_add(req->query, key, value, OD_DUP_ALL);
		}
	}
	free(req->fullpath);
	req->fullpath=strndup(cleanurl, sizeof(cleanurl));
	req->path=req->fullpath;
	return 1;
}
