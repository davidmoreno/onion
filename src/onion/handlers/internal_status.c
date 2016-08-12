/*
	Onion HTTP server library
	Copyright (C) 2010-2016 David Moreno Montero and others

	This library is free software; you can redistribute it and/or
	modify it under the terms of, at your choice:
	
	a. the Apache License Version 2.0. 
	
	b. the GNU General Public License as published by the 
		Free Software Foundation; either version 2.0 of the License, 
		or (at your option) any later version.
	 
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of both libraries, if not see 
	<http://www.gnu.org/licenses/> and 
	<http://www.apache.org/licenses/LICENSE-2.0>.
	*/

#include <onion/onion.h>
#include <onion/dict.h>
#include <onion/types.h>
#include <onion/types_internal.h>

static void header_write(onion_response *res, const char *key, const char *value, int flags){
  onion_response_printf(res,"<li><b>%s</b> = %s</li>",key,value);
}

// static void session_write(onion_response *res, const char *key, onion_dict *value, int flags){
//   onion_response_printf(res,"<li>%s<ul>",key);
//   
//   onion_dict_preorder(value, header_write, res);
//   
//   onion_response_write0(res, "</ul></li>");
// }

static onion_connection_status onion_internal_handler(void *_, onion_request *req, onion_response *res){
  onion_request_get_session_dict(req);
  onion_response_write_headers(res);
  
  onion_response_write0(res, "<html><head><title>Internal status</title></head><body><h1>This petition</h1><h2>Headers</h2><ul>");
  
  // headers
  onion_dict_preorder(onion_request_get_header_dict(req),header_write,res);
  onion_response_write0(res, "</ul><h2>Cookies</h2>");
  
  onion_request_get_session_dict(req);
  onion_response_printf(res,"<ul><li><b>sessionid</b> = %s</li></lu>",req->session_id);
  
  onion_response_write0(res,"<h1>Answer</h1><ul>");
  onion_dict_preorder(res->headers,header_write,res);
  onion_response_write0(res, "</ul>");
  
  
  // Sessions
//   onion_response_write0(res,"<h1>Sessions and data</h1><ul>");
//   onion_dict_preorder( req->connection.listen_point->server->sessions->sessions, session_write, res);
//   onion_response_write0(res, "</ul>");
  
  onion_response_write0(res, "</body></html>");
  return OCS_PROCESSED;
}


onion_handler *onion_internal_status(){
  onion_handler *handler=onion_handler_new(onion_internal_handler, NULL, NULL);
  return handler;
}
