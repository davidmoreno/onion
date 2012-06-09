/*
	Onion HTTP server library
	Copyright (C) 2012 David Moreno Montero

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 3.0 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not see <http://www.gnu.org/licenses/>.
	*/

#ifndef __ONION_CONNECTION_H__
#define __ONION_CONNECTION_H__

#include <onion/types.h>

onion_connection *onion_connection_new(onion_listen_point *op);
void onion_connection_free(onion_connection *oc);
onion_request *onion_connection_request(onion_connection *op);
int onion_connection_accept(onion_listen_point *p);

#endif
