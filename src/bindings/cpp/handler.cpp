/*
	Onion HTTP server library
	Copyright (C) 2010-2014 David Moreno Montero and othes

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

#include "handler.hpp"
#include "request.hpp"
#include "response.hpp"
#include <onion/shortcuts.h>

static onion_connection_status onion_handler_call_operator(void *ptr, onion_request *_req, onion_response *_res){
	try{
		Onion::Handler *handler=(Onion::Handler*)ptr;
		Onion::Request req(_req);
		Onion::Response res(_res);
		return (*handler)(req, res);
	}
	catch(const Onion::HttpInternalError &e){
		return onion_shortcut_response(e.what(), HTTP_INTERNAL_ERROR, _req, _res);
	}
	catch(const std::exception &e){
		ONION_ERROR("Catched exception: %s", e.what());
		return OCS_INTERNAL_ERROR;
	}
}

static void onion_handler_delete_operator(void *ptr){
	Onion::Handler *handler=(Onion::Handler*)ptr;
	delete handler;
}

Onion::Handler::Handler()
{
	ptr=onion_handler_new(onion_handler_call_operator, this, onion_handler_delete_operator);
}

Onion::Handler::~Handler()
{

}

onion_connection_status Onion::HandlerCFunction::operator()(Onion::Request &req, Onion::Response &res){
	return fn(NULL, req.c_handler(), res.c_handler());
}
