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

	onion_dict *d=onion_dict_dup( context.c_handler() );
	
	fn(d, res.c_handler());
	
	return OCS_PROCESSED;
}

onion_connection_status Onion::redirect(const std::string& url, ::Onion::Request& req, ::Onion::Response& res)
{
	return onion_shortcut_redirect(url.c_str(), req.c_handler(), res.c_handler());
}

::Onion::ExportLocal::ExportLocal(const std::string& path) 
{
	export_local=onion_handler_export_local_new(path.c_str());
}

::Onion::ExportLocal::~ExportLocal()
{
	onion_handler_free(export_local);
}

onion_connection_status Onion::ExportLocal::operator()(::Onion::Request &req, ::Onion::Response &res)
{
	return onion_handler_handle(export_local, req.c_handler(), res.c_handler());
}
