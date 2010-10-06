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

#ifndef __ONION_RESPONSE__
#define __ONION_RESPONSE__

#ifdef __cplusplus
extern "C"{
#endif

#include "onion_dict.h"

/**
 * @short The response
 */
struct onion_response_t{
	onion_dict *headers;
	int code;
	
	void *data_generator;
	void *priv_data;
	void *priv_data_delete;
};

typedef struct onion_response_t onion_response;

typedef unsigned int (*onion_data_generator)(onion_response *response, char *data, unsigned int max_length);

#ifdef __cplusplus
}
#endif

#endif
