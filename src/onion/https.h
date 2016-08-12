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

#ifndef ONION_HTTPS_H
#define ONION_HTTPS_H

#include <stdarg.h>
#include "types.h"

#ifdef __cplusplus
extern "C"{
#endif

onion_listen_point *onion_https_new();
int onion_https_set_certificate(onion_listen_point *ol, onion_ssl_certificate_type type, const char *filename, ...);
int onion_https_set_certificate_argv(onion_listen_point *ol, onion_ssl_certificate_type type, const char *filename, va_list va);
#ifdef __cplusplus
}
#endif

#endif
