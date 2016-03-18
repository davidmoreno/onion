/*
	Onion HTTP server library
	Copyright (C) 2010-2016 David Moreno Montero and othes

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

	You should have received a copy of both licenses, if not see 
	<http://www.gnu.org/licenses/> and 
	<http://www.apache.org/licenses/LICENSE-2.0>.
	*/

#include "extrahandlers.hpp"
#include "request.hpp"
#include "response.hpp"
#include "response.hpp"

#include <onion/shortcuts.h>

using namespace Onion;

#ifndef ONION_HAS_LAMBDAS
	struct BoundFunction {
		BoundFunction(const std::string& data, onion_connection_status (*func)(const char*, onion_request*, onion_response*))
			: data(data) {
			this->func = func;
		}
		onion_connection_status operator()(Onion::Request& req, Onion::Response& res) {
			return onion_shortcut_response_file(data.c_str(), req.c_handler(), res.c_handler());
		}
		onion_connection_status (*func)(const char*, onion_request*, onion_response*);
		std::string data;
	};
#endif

Handler Shortcuts::static_file(const std::string& path)
{
#ifdef ONION_HAS_LAMBDAS
	return Handler::make<HandlerFunction>([path](Onion::Request &req, Onion::Response &res){
		return onion_shortcut_response_file(path.c_str(), req.c_handler(), res.c_handler()); 
	});
#else
	std::shared_ptr<BoundFunction> bf { new BoundFunction(path, &onion_shortcut_response_file) };
	return Handler::make<HandlerFunction>(std::bind(&BoundFunction::operator(), bf, std::placeholders::_1, std::placeholders::_2));
#endif
}

Handler Shortcuts::internal_redirect(const std::string& uri)
{
#ifdef ONION_HAS_LAMBDAS
	return Handler::make<HandlerFunction>([uri](Onion::Request &req, Onion::Response &res){
		return onion_shortcut_internal_redirect(uri.c_str(), req.c_handler(), res.c_handler());
	});
#else
	std::shared_ptr<BoundFunction> bf { new BoundFunction(uri, &onion_shortcut_internal_redirect) };
	return Handler::make<HandlerFunction>(std::bind(&BoundFunction::operator(), bf, std::placeholders::_1, std::placeholders::_2));
#endif
}

Handler Shortcuts::redirect(const std::string& uri)
{
#ifdef ONION_HAS_LAMBDAS
	return new HandlerFunction([uri](Onion::Request &req, Onion::Response &res){
		return onion_shortcut_redirect(uri.c_str(), req.c_handler(), res.c_handler());
	});
#else
	std::shared_ptr<BoundFunction> bf { new BoundFunction(uri, &onion_shortcut_redirect) };
	return Handler::make<HandlerFunction>(std::bind(&BoundFunction::operator(), bf, std::placeholders::_1, std::placeholders::_2));
#endif
}

Handler StaticHandler(const std::string &path)
{
	return Shortcuts::static_file(path);
}

Handler InternalRedirectHandler(const std::string& uri)
{
	return Shortcuts::internal_redirect(uri);
}

Handler RedirectHandler(const std::string& uri)
{
	return Shortcuts::redirect(uri);
}

