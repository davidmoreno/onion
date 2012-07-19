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

#include <malloc.h>
#include <string.h>

#include <onion/log.h>
#include <onion/http.h>
#include <onion/types.h>
#include <onion/types_internal.h>
#include <onion/request.h>
#include <onion/block.h>

static void oblp_onion_request_close(onion_request *lp){
	onion_block_free(lp->connection.user_data);
	onion_request_free(lp);
}

static ssize_t oblp_write_append(onion_request *a, const char *b, size_t size){
	ONION_DEBUG("Write %d bytes: %s",size,b);
	onion_block_add_data(a->connection.user_data,b,size);
	return size;
}

static void oblp_listen(onion_listen_point *lp){
	ONION_DEBUG("Empty listen for buffer listen point.");
	return;
}

static void oblp_onion_request_init(onion_request *req){
	ONION_DEBUG("Empty init.");
	req->connection.user_data=onion_block_new();
}

const char* onion_buffer_listen_point_get_buffer_data(onion_request* req)
{
		return onion_block_data(req->connection.user_data);
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
