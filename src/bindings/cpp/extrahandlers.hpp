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

#ifndef ONION_EXTRAHANDLERS_HPP
#define ONION_EXTRAHANDLERS_HPP

#include <string>
#include <string.h>
#include <onion/shortcuts.h>

#include "handler.hpp"
#include "request.hpp"
#include "response.hpp"

namespace Onion{
	class HandlerCBridge : public Handler{
	public:
		typedef void (*free_t)(void *data);
		typedef onion_connection_status (*f_t)(void *data, onion_request *req, onion_response *res);
		
		HandlerCBridge(f_t c_handler_f) : f(c_handler_f), ud(NULL), fud(NULL) {};
		HandlerCBridge(f_t c_handler_f, void *userdata, free_t free_userdata=NULL) : f(c_handler_f), ud(userdata), fud(free_userdata){};
		virtual ~HandlerCBridge(){
			if (fud)
				fud(ud);
		}
		virtual onion_connection_status operator()(Request &req, Response &res){
			return f(ud, req.c_handler(), res.c_handler());
		}
	private:
		f_t f;
		void *ud;
		free_t fud;
	};
	
	class StaticHandler : public Handler{
		std::string path;
	public:
		StaticHandler(const std::string &path);
		virtual ~StaticHandler();
		
		virtual onion_connection_status operator()(Request& , Response& );
	};
	
	class InternalRedirectHandler : public HandlerCBridge{
	public:
		InternalRedirectHandler(const std::string &uri) : HandlerCBridge(f_t(onion_shortcut_internal_redirect), (void*)strdup(uri.c_str()), free_t(free)){}
		InternalRedirectHandler(const char *uri) : HandlerCBridge(f_t(onion_shortcut_internal_redirect), (void*)uri){}
	};
	
	class RedirectHandler : public HandlerCBridge{
	public:
		RedirectHandler(const std::string &uri) : HandlerCBridge(f_t(onion_shortcut_redirect), (void*)strdup(uri.c_str()), free_t(free)){}
		RedirectHandler(const char *uri) : HandlerCBridge(f_t(onion_shortcut_redirect), (void*)uri){}
	};
};

#endif

