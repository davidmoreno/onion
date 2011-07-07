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
#include <stdlib.h>
#include <unistd.h>

#include "server.h"
#include "dict.h"
#include "request.h"
#include "response.h"
#include "handler.h"
#include "server.h"
#include "types_internal.h"
#include "log.h"
#include "sessions.h"

void onion_request_parser_data_free(void *token); // At request_parser.c

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
	if (client_info) // This is kept even on clean
		req->client_info=strdup(client_info);
	else
		req->client_info=NULL;

	return req;
}

/**
 * @short Helper to remove temporal files from req->files
 */
static void unlink_files(void *p, const char *key, const char *value, int flags){
	ONION_DEBUG0("Unlinking temporal file %s",value);
	unlink(value);
}

/// Deletes a request and all its data
void onion_request_free(onion_request *req){
	onion_dict_free(req->headers);
	
	if (req->parser_data){
		onion_request_parser_data_free(req->parser_data);
		req->parser_data=NULL;
	}
	
	if (req->fullpath)
		free(req->fullpath);
	if (req->GET)
		onion_dict_free(req->GET);
	if (req->POST)
		onion_dict_free(req->POST);
	if (req->FILES){
		onion_dict_preorder(req->FILES, unlink_files, NULL);
		onion_dict_free(req->FILES);
	}
	if (req->parser_data)
		free(req->parser_data);
	if (req->client_info)
		free(req->client_info);
	if (req->session_id)
		free(req->session_id);
	if (req->session)
		onion_dict_free(req->session); // Not really remove, just dereference
	
	free(req);
}


/// Returns a pointer to the string with the current path. Its a const and should not be trusted for long time.
const char *onion_request_get_path(onion_request *req){
	return req->path;
}

/// Gets the current flags, as in onion_request_flags_e
onion_request_flags onion_request_get_flags(onion_request *req){
	return req->flags;
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
	if (req->GET)
		return onion_dict_get(req->GET, query);
	return NULL;
}

/// Gets a query data, but has a default value if the key is not there.
const char *onion_request_get_queryd(onion_request *req, const char *key, const char *def){
	const char *ret;
	ret=onion_request_get_query(req,key);
	if (ret)
		return ret;
	return def;
}

/// Gets a post data
const char *onion_request_get_post(onion_request *req, const char *query){
	if (req->POST)
		return onion_dict_get(req->POST, query);
	return NULL;
}

/// Gets file data
const char *onion_request_get_file(onion_request *req, const char *query){
	if (req->FILES)
		return onion_dict_get(req->FILES, query);
	return NULL;
}

/// Gets session data
const char *onion_request_get_session(onion_request *req, const char *key){
	onion_dict *d=onion_request_get_session_dict(req);
	return onion_dict_get(d, key);
}

/// Gets the header header data dict
const onion_dict *onion_request_get_header_dict(onion_request *req){
	return req->headers;
}

/// Gets request query dict
const onion_dict *onion_request_get_query_dict(onion_request *req){
	return req->GET;
}

/// Gets post data dict
const onion_dict *onion_request_get_post_dict(onion_request *req){
	return req->POST;
}

/// Gets files data dict
const onion_dict *onion_request_get_file_dict(onion_request *req){
	return req->FILES;
}

/**
 * @short Gets the sessionid cookie, if any, and sets it to req->session_id.
 */
void onion_request_guess_session_id(onion_request *req){
	if (req->session_id) // already known.
		return;
	const char *v=onion_dict_get(req->headers, "Cookie");
	ONION_DEBUG("Session ID, maybe from %s",v);
	if (!v)
		return;
	v=strstr(v,"sessionid=");
	if (!v)
		return;
	char *r=strdup(v+10); // Maybe allocated more memory. FIXME. Not much anyway.
	char *p=r;
	while (*p!='\0' && *p!=';') p++;
	*p='\0';
	req->session_id=r;
	ONION_DEBUG("Session ID, from cookie, is %s",req->session_id);
}

/**
 * @short Returns the session dict.
 * 
 * If it does not exists it creates it. If there is a cookie with a proper name it is used, 
 * even for creation.
 * 
 * Returned dictionary can be freely managed (added new keys...) and this is the session data.
 */
onion_dict *onion_request_get_session_dict(onion_request *req){
	if (!req->session){
		onion_request_guess_session_id(req);
		if (req->session_id)
			req->session=onion_sessions_get(req->server->sessions, req->session_id);
		if (!req->session){ // Maybe old session is not to be used anymore
			req->session_id=onion_sessions_create(req->server->sessions);
			req->session=onion_sessions_get(req->server->sessions, req->session_id);
		}
	}
	return req->session;
}


/**
 * @short Cleans a request object to reuse it.
 */
void onion_request_clean(onion_request* req){
	onion_dict_free(req->headers);
	req->headers=onion_dict_new();
	if (req->parser_data){
		onion_request_parser_data_free(req->parser_data);
		req->parser_data=NULL;
	}
	req->parser=NULL;
	req->flags&=0x0F00; // I keep server flags.
	if (req->fullpath){
		free(req->fullpath);
		req->path=req->fullpath=NULL;
	}
	if (req->GET){
		onion_dict_free(req->GET);
		req->GET=NULL;
	}
	if (req->POST){
		onion_dict_free(req->POST);
		req->POST=NULL;
	}
	if (req->FILES){
		onion_dict_preorder(req->FILES, unlink_files, NULL);
		onion_dict_free(req->FILES);
		req->FILES=NULL;
	}
}

/**
 * @short Forces the request to process only one request, not doing the keep alive.
 * 
 * This is useful on non threaded modes, as the keep alive blocks the loop.
 */
void onion_request_set_no_keep_alive(onion_request *req){
	req->flags|=OR_NO_KEEP_ALIVE;
//	ONION_DEBUG("Disabling keep alive %X",req->flags);
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


/** 
 * @short Frees the session dictionary.
 * 
 * If data is under onion_dict scope (just dicts into dicts and strings), all data is freed.
 * If the user has set some custom data, THAT MEMORY IS LEAKED.
 */
void onion_request_session_free(onion_request *req){
	if (!req->session_id)
		onion_request_guess_session_id(req);
	if (req->session_id){
		onion_sessions_remove(req->server->sessions, req->session_id);
		onion_dict_free(req->session);
		req->session=NULL;
		free(req->session_id);
		req->session_id=NULL;
	}
}

/**
 * @short Returns the language code of the current request
 * 
 * Returns the language code for the current request, from the header. 
 * If none the returns "C". 
 * 
 * Language code is short code. No localization by the moment.
 */
const char *onion_request_get_language_code(onion_request *req){
	const char *lang=onion_dict_get(req->headers, "Accept-Language");
	if (lang){
		char *l=strdup(lang);
		char *p=l;
		while (*p){ // search first part
			if (*p=='-' || *p==';' || *p==','){
				*p=0;
				break;
			}
			p++;
		}
		//ONION_DEBUG("Language is %s", l);
		return l;
	}
	return "C";
}

