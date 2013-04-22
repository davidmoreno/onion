/*
	Onion HTTP server library
	Copyright (C) 2012 David Moreno Montero

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

#include <stdlib.h>
#include <string.h>

#include <onion/log.h>
#include <onion/http.h>
#include <onion/types.h>
#include <onion/types_internal.h>
#include <onion/request.h>
#include <onion/block.h>

static void oblp_onion_request_close(onion_request *req){
	ONION_DEBUG("Free onion buffer listen point");
	onion_block_free(req->connection.user_data);
}

ssize_t oblp_write_append(onion_request *a, const char *b, size_t size){
	ONION_DEBUG("Write %d bytes.",size);
	onion_block_add_data(a->connection.user_data,b,size);
	return size;
}

static void oblp_listen(onion_listen_point *lp){
	ONION_DEBUG("Empty listen for buffer listen point.");
	return;
}

static int oblp_onion_request_init(onion_request *req){
	ONION_DEBUG("Empty init.");
	req->connection.user_data=onion_block_new();
	return 0;
}

const char* onion_buffer_listen_point_get_buffer_data(onion_request* req)
{
		return onion_block_data(req->connection.user_data);
}

onion_block* onion_buffer_listen_point_get_buffer(onion_request* req)
{
		return req->connection.user_data;
}

onion_listen_point* onion_buffer_listen_point_new()
{
	onion_listen_point *lp=onion_http_new();
	lp->request_init=oblp_onion_request_init;
	lp->write=oblp_write_append;
	lp->close=oblp_onion_request_close;
	lp->listen=oblp_listen;
	return lp;
}
