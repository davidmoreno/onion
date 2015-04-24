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

#include <onion/onion.h>
#include <onion/http.h>
#include <onion/websocket.h>
#include "../ctest.h"
#include "buffer_listen_point.h"

struct ws_status_t{
	int connected;
	int is_connected;
};
struct ws_status_t ws_status;

onion_connection_status ws_callback(void *privadata, onion_websocket *ws, ssize_t nbytes_ready){
	return OCS_NEED_MORE_DATA;
}

onion_connection_status ws_handler(void *priv, onion_request *req, onion_response *res){
	onion_websocket *ws=onion_websocket_new(req, res);
	ws_status.connected++;
	ws_status.is_connected=(ws!=NULL);
	
	if (ws_status.is_connected){
		onion_websocket_set_callback(ws, ws_callback);
		return OCS_WEBSOCKET;
	}
	return OCS_NOT_IMPLEMENTED;
}

onion *websocket_server_new(){
	onion *o=onion_new(0);
	onion_url *urls=onion_root_url(o);
	onion_url_add(urls, "", ws_handler);
	
	onion_listen_point *lp=onion_buffer_listen_point_new();
	onion_add_listen_point(o,NULL,NULL,lp);
	
	return o;
}

int onion_request_write0(onion_request *req, const char *data){
	return onion_request_write(req,data,strlen(data));
}

void t01_websocket_server_no_ws(){
	INIT_LOCAL();
	
	memset(&ws_status,0,sizeof(ws_status));
	onion *o=websocket_server_new();
	onion_request *req=onion_request_new(onion_get_listen_point(o, 0));
	onion_request_write0(req,"GET /\n\n");
	FAIL_IF(ws_status.is_connected);
	FAIL_IF_NOT_EQUAL_INT(ws_status.connected,1);
	
	onion_request_free(req);
	onion_free(o);
	
	END_LOCAL();
}

void t02_websocket_server_w_ws(){
	INIT_LOCAL();
	
	memset(&ws_status,0,sizeof(ws_status));
	onion *o=websocket_server_new();
	onion_request *req=onion_request_new(onion_get_listen_point(o, 0));
	onion_request_write0(req,"GET /\nUpgrade: websocket\nSec-Websocket-Version: 13\nSec-Websocket-Key: My-key\n\n");
	FAIL_IF_NOT(ws_status.is_connected);
	FAIL_IF_NOT_EQUAL_INT(ws_status.connected,1);
	
	onion_request_free(req);
	onion_free(o);
	
	END_LOCAL();
}


int main(int argc, char **argv){
	START();
	
	t01_websocket_server_no_ws();
	t02_websocket_server_w_ws();
	
	END();
}
