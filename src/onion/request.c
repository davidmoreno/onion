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
#include <ctype.h>

#include "server.h"
#include "dict.h"
#include "request.h"
#include "response.h"
#include "handler.h"
#include "server.h"
#include "types_internal.h"
#include "log.h"
#include "sessions.h"
#include "block.h"

void onion_request_parser_data_free(void *token); // At request_parser.c

/**
 * @memberof onion_request_t
 * These are the methods allowed to ask data to the server (or push or whatever). Only 16.
 * 
 * NULL means this space is vacant.
 * 
 * They are in order of probability, with GET the most common.
 */
const char *onion_request_methods[16]={ 
	"GET", "POST", "HEAD", "OPTIONS", 
	"PROPFIND", "PUT", "DELETE", "MOVE", 
	"MKCOL", "PROPPATCH", NULL, NULL, 
	NULL, NULL, NULL, NULL };

/**
 *  @short Creates a request object
 * @memberof onion_request_t
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

  ONION_DEBUG0("Create request %p", req);
	return req;
}

/**
 * @short Helper to remove temporal files from req->files
 * @memberof onion_request_t
 */
static void unlink_files(void *p, const char *key, const char *value, int flags){
	ONION_DEBUG0("Unlinking temporal file %s",value);
	unlink(value);
}

/**
 * @short Deletes a request and all its data
 * @memberof onion_request_t
 */
void onion_request_free(onion_request *req){
  ONION_DEBUG0("Free request %p", req);
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
	if (req->session){
    if (onion_dict_count(req->session)==0)
      onion_request_session_free(req);
    else{
      onion_dict_free(req->session); // Not really remove, just dereference
      free(req->session_id);
    }
  }
	if (req->data)
		onion_block_free(req->data);

	free(req);
}

/**
 * @short Cleans a request object to reuse it.
 * @memberof onion_request_t
 */
void onion_request_clean(onion_request* req){
  ONION_DEBUG0("Clean request %p", req);
  onion_dict_free(req->headers);
  req->headers=onion_dict_new();
  if (req->parser_data){
    onion_request_parser_data_free(req->parser_data);
    req->parser_data=NULL;
  }
  req->parser=NULL;
  req->flags&=OR_NO_KEEP_ALIVE; // I keep keep alive.
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
  if (req->session_id){
    if (onion_dict_count(req->session)==0){
      onion_request_session_free(req);
    }
    else{
      onion_dict_free(req->session); // Not really remove, just dereference
      req->session=NULL;
      free(req->session_id);
      req->session_id=NULL;
    }
  }
  if (req->data){
    onion_block_free(req->data);
    req->data=NULL;
  }
}


/**
 * @short Returns a pointer to the string with the current path. Its a const and should not be trusted for long time.
 * @memberof onion_request_t
 */
const char *onion_request_get_path(onion_request *req){
	return req->path;
}

/**
 * @short Returns a pointer to the string with the full path. Its a const and should not be trusted for long time.
 * @memberof onion_request_t
 */
const char *onion_request_get_fullpath(onion_request *req){
	return req->fullpath;
}

/** 
 * @short Gets the current flags, as in onion_request_flags_e
 * @memberof onion_request_t
 */
onion_request_flags onion_request_get_flags(onion_request *req){
	return req->flags;
}

/**
 * @short  Moves the pointer inside fullpath to this new position, relative to current path.
 * @memberof onion_request_t
 */
void onion_request_advance_path(onion_request *req, int addtopos){
	req->path=&req->path[addtopos];
}

/**
 * @short Gets a header data
 * @memberof onion_request_t
 */
const char *onion_request_get_header(onion_request *req, const char *header){
	return onion_dict_get(req->headers, header);
}

/**
 * @short Gets a query data
 * @memberof onion_request_t
 */
const char *onion_request_get_query(onion_request *req, const char *query){
	if (req->GET)
		return onion_dict_get(req->GET, query);
	return NULL;
}

/**
 * @short Gets a query data, but has a default value if the key is not there.
 * @memberof onion_request_t
 */
const char *onion_request_get_queryd(onion_request *req, const char *key, const char *def){
	const char *ret;
	ret=onion_request_get_query(req,key);
	if (ret)
		return ret;
	return def;
}

/**
 * @short Gets a post data
 * @memberof onion_request_t
 */
const char *onion_request_get_post(onion_request *req, const char *query){
	if (req->POST)
		return onion_dict_get(req->POST, query);
	return NULL;
}

/**
 * @short Gets file data
 * @memberof onion_request_t
 */
const char *onion_request_get_file(onion_request *req, const char *query){
	if (req->FILES)
		return onion_dict_get(req->FILES, query);
	return NULL;
}

/**
 * @short Gets session data
 * @memberof onion_request_t
 */
const char *onion_request_get_session(onion_request *req, const char *key){
	onion_dict *d=onion_request_get_session_dict(req);
	return onion_dict_get(d, key);
}

/**
 * @short Gets the header header data dict
 * @memberof onion_request_t
 */
const onion_dict *onion_request_get_header_dict(onion_request *req){
	return req->headers;
}

/**
 * @short Gets request query dict
 * @memberof onion_request_t
 */
const onion_dict *onion_request_get_query_dict(onion_request *req){
	return req->GET;
}

/**
 * @short Gets post data dict
 * @memberof onion_request_t
 */
const onion_dict *onion_request_get_post_dict(onion_request *req){
	return req->POST;
}

/**
 * @short Gets files data dict
 * @memberof onion_request_t
 */
const onion_dict *onion_request_get_file_dict(onion_request *req){
	return req->FILES;
}

/**
 * @short Gets the sessionid cookie, if any, and sets it to req->session_id.
 * @memberof onion_request_t
 */
void onion_request_guess_session_id(onion_request *req){
	if (req->session_id) // already known.
		return;
	const char *ov=onion_dict_get(req->headers, "Cookie");
  const char *v=ov;
	ONION_DEBUG("Session ID, maybe from %s",v);
	char *r=NULL;
	onion_dict *session;
	
	do{ // Check all possible sessions
		if (r){
			free(r);
      r=NULL;
    }
		if (!v)
			return;
		v=strstr(v,"sessionid=");
		if (!v) // exit point, no session found.
			return;
    if (v>ov && isalnum(v[-1])){
      ONION_DEBUG("At -1: %c %d (%p %p)",v[-1],isalnum(v[-1]),v,ov);
      v=strstr(v,";");
    }
    else{
      v+=10;
      r=strdup(v); // Maybe allocated more memory, not much anyway.
      char *p=r;
      while (*p!='\0' && *p!=';') p++;
      *p='\0';
      ONION_DEBUG0("Checking if %s exists in sessions", r);
      session=onion_sessions_get(req->server->sessions, r);
    }
	}while(!session);
	
	req->session_id=r;
	req->session=session;
	ONION_DEBUG("Session ID, from cookie, is %s",req->session_id);
}

/**
 * @short Returns the session dict.
 * @memberof onion_request_t
 * 
 * If it does not exists it creates it. If there is a cookie with a proper name it is used, 
 * even for creation.
 * 
 * Sessions HAVE TO be gotten before sending any header, or user may face double sessionid, ghost sessions and some other
 * artifacts, as the cookie is not set if session is not used. If this condition happen (send headers and then ask for session) a
 * WARNING is written. 
 * 
 * Session is not automatically retrieved as it is a slow operation and not used normally, only on "active" handlers.
 * 
 * Returned dictionary can be freely managed (added new keys...) and this is the session data.
 * 
 * @return session dictionary for current request.
 */
onion_dict *onion_request_get_session_dict(onion_request *req){
	if (!req->session){
    if (req->flags & OR_HEADER_SENT){
      ONION_WARNING("Asking for session AFTER sending headers. This may result in double sessionids, and wrong session behaviour. Please modify your handlers to ask for session BEFORE sending any data.");
    }
		onion_request_guess_session_id(req);
		if (!req->session){ // Maybe old session is not to be used anymore
			req->session_id=onion_sessions_create(req->server->sessions);
			req->session=onion_sessions_get(req->server->sessions, req->session_id);
		}
	}
	return req->session;
}


/**
 * @short Forces the request to process only one request, not doing the keep alive.
 * @memberof onion_request_t
 * 
 * This is useful on non threaded modes, as the keep alive blocks the loop.
 */
void onion_request_set_no_keep_alive(onion_request *req){
	req->flags|=OR_NO_KEEP_ALIVE;
//	ONION_DEBUG("Disabling keep alive %X",req->flags);
}

/**
 * @short Returns if current request wants to keep alive.
 * @memberof onion_request_t
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
 * @memberof onion_request_t
 * 
 * It removes the session from the sessions dictionary, so this session does not exist anymore.
 * 
 * If data is under onion_dict scope (just dicts into dicts and strings), all data is freed.
 * If the user has set some custom data, THAT MEMORY IS LEAKED.
 */
void onion_request_session_free(onion_request *req){
	if (!req->session_id)
		onion_request_guess_session_id(req);
	if (req->session_id){
    ONION_DEBUG("Removing from session storage session id: %s",req->session_id);
		onion_sessions_remove(req->server->sessions, req->session_id);
		onion_dict_free(req->session);
		req->session=NULL;
		free(req->session_id);
		req->session_id=NULL;
	}
}

/**
 * @short Returns the language code of the current request
 * @memberof onion_request_t
 * 
 * Returns the language code for the current request, from the header. 
 * If none the returns "C". 
 * 
 * Language code is short code. No localization by the moment.
 * 
 * @returns The language code for this request or C. Data must be freed.
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
	return strdup("C");
}

/**
 * @short Some extra data, normally when the petition is propfind
 * @memberof onion_request_t
 */
const onion_block *onion_request_get_data(onion_request *req){
	return req->data;
}

/**
 * @short Launches one handler for the given request
 * 
 * Once the request is ready, launch it.
 * 
 * @returns The connection status: if it should be closed, error codes...
 */
onion_connection_status onion_request_process(onion_request *req){
	return onion_server_handle_request(req->server, req);
}

/**
 * @short Performs the final touches do the request is ready to be handled.
 * 
 * It sets the current path.
 * 
 * @param req The request.
 */
void onion_request_polish(onion_request *req){
  if (*req->fullpath)
    req->path=req->fullpath+1;
  else
    req->path=req->fullpath;
}
