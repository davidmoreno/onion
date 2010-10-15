/*
	Onion HTTP server library
	Copyright (C) 2010 David Moreno Montero

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Affero General Public License as
	published by the Free Software Foundation, either version 3 of the
	License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Affero General Public License for more details.

	You should have received a copy of the GNU Affero General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
	*/

#ifndef __ONION_HANDLER_STATIC__
#define __ONION_HANDLER_STATIC__

#include <onion_types.h>

#ifdef __cplusplus
extern "C"{
#endif

/// Creates an static handler. Returns some content from memory, like an error html.
onion_handler *onion_handler_static(const char *path, const char *text, int code);

#ifdef __cplusplus
}
#endif

#endif
