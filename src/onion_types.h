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

#ifndef __ONION_TYPES__
#define __ONION_TYPES__

struct onion_dict_t;
typedef struct onion_dict_t onion_dict;
struct onion_handler_t;
typedef struct onion_handler_t onion_handler;
struct onion_request_t;
typedef struct onion_request_t onion_request;
struct onion_response_t;
typedef struct onion_response_t onion_response;
struct onion_server_t;
typedef struct onion_server_t onion_server;


typedef int (*onion_parser)(onion_handler *handler, onion_request *);
typedef int (*onion_handler_private_data_free)(void *privdata);

/**
 * @short Prototype for the writing on the socket function.
 *
 * It can safely be just fwrite, with handler the FILE*, or write with the handler the fd.
 * But its usefull its like this so we can use another more complex to support, for example,
 * SSL.
 */
typedef int (*onion_write)(void *handler, const char *data, unsigned int length);



#endif

