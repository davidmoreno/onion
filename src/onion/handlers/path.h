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

#ifndef __ONION_HANDLER_PATH__
#define __ONION_HANDLER_PATH__

#include <onion/types.h>

#ifdef __cplusplus
extern "C"{
#endif

/// Creates an path handler. If the path matches the regex, it reomves that from the regexp and goes to the inside_level.
onion_handler *onion_handler_path(const char *path, onion_handler *inside_level);

#ifdef __cplusplus
}
#endif

#endif
