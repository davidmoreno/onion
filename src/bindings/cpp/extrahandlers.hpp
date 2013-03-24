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

#ifndef __EXTRAHANDLERS_HPP__
#define __EXTRAHANDLERS_HPP__

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

