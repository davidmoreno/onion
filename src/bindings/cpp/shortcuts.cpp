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
#include "shortcuts.hpp"
#include "onion.hpp"
#include "dict.hpp"
#include "response.hpp"
#include "request.hpp"
#include <onion/dict.h>
#include <onion/log.h>
#include <onion/shortcuts.h>
#include <onion/handler.h>
#include <onion/handlers/exportlocal.h>

onion_connection_status Onion::render_to_response(::Onion::template_f fn, const ::Onion::Dict& context, ::Onion::Response &res){
	ONION_DEBUG("Context: %s", context.toJSON().c_str());

	fn(context.c_handler(), res.c_handler());
	
	return OCS_PROCESSED;
}

onion_connection_status Onion::redirect(const std::string& url, ::Onion::Request& req, ::Onion::Response& res)
{
	return onion_shortcut_redirect(url.c_str(), req.c_handler(), res.c_handler());
}

Onion::Handler Onion::ExportLocal(const std::string& path)
{
	return onion_handler_c_to_cpp(onion_handler_export_local_new(path.c_str()));
}
