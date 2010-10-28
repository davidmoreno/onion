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

#ifndef __ONION_HANDLER__
#define __ONION_HANDLER__

#ifdef __cplusplus
extern "C"{
#endif

#include "onion_request.h"
#include "onion_types.h"

/// checks that handler to handle the request
int onion_handler_handle(onion_handler *handler, onion_request *request);

/// Creates an onion handler with that private datas.
onion_handler *onion_handler_new(onion_handler_handler handler, void *privdata, onion_handler_private_data_free priv_data_free);

/// Frees the memory of the handler
int onion_handler_free(onion_handler *handler);

/// Adds a handler to the list of handler of this level
void onion_handler_add(onion_handler *base, onion_handler *new_handler);

#ifdef __cplusplus
}
#endif

#endif
