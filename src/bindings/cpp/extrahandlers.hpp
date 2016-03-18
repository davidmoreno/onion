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
#include "features.hpp"

namespace Onion{

	namespace Shortcuts{
		Handler static_file(const std::string &path);
		Handler internal_redirect(const std::string &uri);
		Handler redirect(const std::string &uri);
	};

	Handler StaticHandler(const std::string &path);
	Handler InternalRedirectHandler(const std::string &uri);
	Handler RedirectHandler(const std::string &uri);
};

#endif

