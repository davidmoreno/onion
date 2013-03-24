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

#ifndef __ONION_LISTEN_POINT_H__
#define __ONION_LISTEN_POINT_H__

#include "types.h"

onion_listen_point *onion_listen_point_new();
int onion_listen_point_listen(onion_listen_point *);
void onion_listen_point_listen_stop(onion_listen_point *op);
void onion_listen_point_free(onion_listen_point *);
int onion_listen_point_accept(onion_listen_point *);
int onion_listen_point_request_init_from_socket(onion_request *op);
void onion_listen_point_request_close_socket(onion_request *oc);

#endif
