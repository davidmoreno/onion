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

#ifndef ONION_WEBSOCKETS_H
#define ONION_WEBSOCKETS_H


#ifdef __cplusplus
extern "C"{
#endif

#include <stddef.h>
#include <stdarg.h>
#include "types.h"


/// Get the current websocket handler, or create it. If not a websocket request, returns NULL
onion_websocket *onion_websocket_new(onion_request *req, onion_response *res);
/// When freed, the callback is invoked with a negative data length.
void onion_websocket_free(onion_websocket *ws);
void onion_websocket_close(onion_websocket *ws, const char *status);

void onion_websocket_set_callback(onion_websocket *ws, onion_websocket_callback_t cb);
void onion_websocket_set_userdata(onion_websocket *ws, void *userdata, void (*free_userdata)(void *));
int onion_websocket_read(onion_websocket *ws, char *buffer, size_t len);
int onion_websocket_write(onion_websocket *ws, const char *buffer, size_t len);
int onion_websocket_vprintf(onion_websocket *ws, const char *fmt, va_list args)
  __attribute__ ((format (printf, 2, 0)));
int onion_websocket_printf(onion_websocket *ws, const char *str, ...)
  __attribute__ ((format (printf, 2, 3)));
onion_connection_status onion_websocket_call(onion_websocket *ws);
void onion_websocket_set_opcode(onion_websocket *ws, onion_websocket_opcode opcode);
onion_websocket_opcode onion_websocket_get_opcode(onion_websocket *ws);

#ifdef __cplusplus
}
#endif

#endif
