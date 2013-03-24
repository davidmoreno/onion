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

#ifndef __WEBSOCKETS_H__
#define __WEBSOCKETS_H__


#ifdef __cplusplus
extern "C"{
#endif

#include <stddef.h>
#include "types.h"


/// Get the current websocket handler, or create it. If not a websocket request, returns NULL
onion_websocket *onion_websocket_new(onion_request *req, onion_response *res);
void onion_websocket_free(onion_websocket *ws);

void onion_websocket_set_callback(onion_websocket *ws, onion_websocket_callback_t cb);
int onion_websocket_read(onion_websocket *ws, char *buffer, size_t len);
int onion_websocket_write(onion_websocket *ws, const char *buffer, size_t len);
int onion_websocket_printf(onion_websocket *ws, const char *str, ...);
onion_connection_status onion_websocket_call(onion_websocket *ws);
void onion_websocket_set_opcode(onion_websocket *ws, onion_websocket_opcode opcode);
onion_websocket_opcode onion_websocket_get_opcode(onion_websocket *ws);

#ifdef __cplusplus
}
#endif

#endif
