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

#ifndef __ONION__
#define __ONION__

#ifdef __cplusplus
extern "C"{
#endif

#include "onion_request.h"

/**
 * @short Information about a handler for onion. A tree structure of handlers is what really serves the data.
 */
struct onion_handler_t{
	const char *name;        /// Informatory only.
	
	void *handler;            /// callback that should return an onion_response object, or NULL if im not entitled to respnse this request.
	void *priv_data;         /// Private data as needed by the parser
	void *priv_data_delete;  /// When freeing some memory, how to remove the private memory.
	
	struct onion_handler_t *next; /// If parser returns null, i try next parser. If no next parser i go up, or return an error. @see onion_tree_parser
};

typedef struct onion_handler_t onion_handler;

typedef int (*onion_parser)(onion_handler *handler, onion_request *);


/// checks that handler to handle the request
int onion_handler_handle(onion_handler *handler, onion_request *request);


#ifdef __cplusplus
}
#endif

#endif
