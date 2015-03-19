/*
	Onion HTTP server library
	Copyright (C) 2010-2015 David Moreno Montero and others

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

#include <onion/shortcuts.h>

#include "exceptions.hpp"
#include "response.hpp"
#include "request.hpp"

using namespace Onion;

onion_connection_status Onion::HttpException::handle(Onion::Request& req, Onion::Response& res)
{
	return onion_shortcut_response(what(), code, req.c_handler(), res.c_handler());
}

onion_connection_status Onion::HttpRedirect::handle(Onion::Request& req, Onion::Response& res)
{
	return onion_shortcut_redirect(what(), req.c_handler(), res.c_handler());
}

