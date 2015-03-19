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

#pragma once

#include "onion.hpp"

namespace Onion{
	class Dict;
	class Request;
	class Response;
	
	typedef std::function<void (onion_dict *d, onion_response *r)> template_f;
 
	/// Renders a otemplate template with the given context, on this response.
	onion_connection_status render_to_response(template_f fn, const Dict& context, Response &res);
	
	/// Redirects to an given url
	onion_connection_status redirect(const std::string &url, Request &req, Response &res);
	
	
	/**
	 * @short Exports local data as a handler
	 * 
	 * Using this class developer can safetly export a local directory.
	 * 
	 * Example to export current directory:
	 * 
	 * @code
	 * Onion::Onion o;
	 * o.setRootHandler(ExportLocal("."));
	 * o.listen();
	 * @endcode
	 */
	Handler ExportLocal(const std::string &path);
}
