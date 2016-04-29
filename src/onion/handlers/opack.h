/*
	Onion HTTP server library
	Copyright (C) 2010-2016 David Moreno Montero and others

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

#ifndef __ONION_HANDLER_OPACK__
#define __ONION_HANDLER_OPACK__

#include <onion/types.h>

#ifdef __cplusplus
extern "C"{
#endif

typedef void (*onion_opack_renderer)(onion_response *res);

/// Creates a opak handler.
onion_handler *onion_handler_opack(const char *path, onion_opack_renderer opack, unsigned int length);

#ifdef __cplusplus
}
#endif

#endif
