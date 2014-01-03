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

#include "extrahandlers.hpp"
#include "request.hpp"
#include "response.hpp"
#include <onion/shortcuts.h>

using namespace Onion;

StaticHandler::StaticHandler(const std::string& _path) : path(_path)
{
	
}

StaticHandler::~StaticHandler()
{

}

onion_connection_status StaticHandler::operator()(Request &req, Response &res)
{
	return onion_shortcut_response_file((path+req.path()).c_str(), req.c_handler(), res.c_handler());
}
