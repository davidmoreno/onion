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
#include <onion/types_internal.h>
#include "../ctest.h"
#include "buffer_listen_point.h"

struct ws_status_t{
	int connected;
	int is_connected;
};
struct ws_status_t ws_status;

char *ws_data_tmp = NULL;
int ws_data_length = 0;

typedef ssize_t (lpreader_sig_t)(onion_request *req, char *data, size_t len);
typedef ssize_t (lpwriter_sig_t)(onion_request *req, const char *data, size_t len);

void websocket_data_buffer_write(onion_request *req, const char *data, size_t len){
	if(!ws_data_tmp){
		ws_data_length = len;
		ws_data_tmp = malloc(ws_data_length);
		strncpy(ws_data_tmp, data, ws_data_length);
	}
	else {
		char *tmp = malloc(ws_data_length+len);
		strncpy(tmp, ws_data_tmp, ws_data_length);
		strncpy(tmp+ws_data_length, data, len);
		ws_data_length +=len;
		free(ws_data_tmp);
		ws_data_tmp = tmp;
	}
}

int websocket_data_buffer_read(onion_request *req, char *data, size_t len){
	if(!ws_data_tmp || !ws_data_length || !data) return 0;
	int i;
	for(i=0; i < ws_data_length; i++){
		data[i] = ws_data_tmp[i];
		if(i == len-1) break;
	}
	strncpy(ws_data_tmp, ws_data_tmp+i+1, ws_data_length-i);
	ws_data_length -= i+1;
	return i+1;
}

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

int websocket_forge_small_packet(char **buffer){
	// Forge 120 bytes Data Packet
	int length=120;
	int header_length = 6;
	*buffer = malloc(sizeof(char) * length);
	char *buf = *buffer;
	memset(*buffer, '\0', length);
	buf[0]=0x81; // FIN flag to 1, opcode: 0x01; one message, of type text;
	buf[1]=0x1 << 7; // MASK Flag to 1;
	buf[1]+=length-header_length; // length: 113 bytes of Data;
	// Next 4 bytes: Masking-key
	buf[2]=0xF2;
	buf[3]=0x05;
	buf[4]=0xA0;
	buf[5]=0x01;
	// actual data
	strncpy(&(buf[6]), "Some UTF-8-encoded chars which will be cut at the 117th char so I write some gap-filling text with no meaning until it is enough", length-header_length);

	// Masking operation on data
	int i;
	for(i=0; i < length-header_length; i++){
		buf[i+6]=buf[i+6] ^ buf[2+i%4];
	}
	fflush(stdout);
	return length;
}

int websocket_forge_close_packet(char **buffer){
	// Forge 120 bytes Data Packet
	int length=8;
	int header_length = 6;
	*buffer = malloc(sizeof(char) * length);
	char *buf = *buffer;
	memset(*buffer, '\0', length);
	buf[0]=0x88; // FIN flag to 1, opcode: 0x08
	buf[1]=0x1 << 7; // MASK Flag to 1;
	buf[1]+=length-header_length; // length: 2 bytes of Data;
	// Next 4 bytes: Masking-key
	buf[2]=0xF2;
	buf[3]=0x05;
	buf[4]=0xA0;
	buf[5]=0x01;
	// Status code masked with key:
	buf[6]=0x03 ^ buf[2];
	buf[7]=0xE8 ^ buf[3];

	return length;
}

int onion_request_write0(onion_request *req, const char *data){
	return onion_request_write(req,data,strlen(data));
}

onion_request *websocket_start_handshake(onion *o){
	onion_request *req=onion_request_new(onion_get_listen_point(o, 0));
	onion_request_write0(req,"GET /\nUpgrade: websocket\nSec-Websocket-Version: 13\nSec-Websocket-Key: My-key\n\n");
	onion_request_process(req);
	return req;
}

void t01_websocket_server_no_ws(){
	INIT_LOCAL();

	memset(&ws_status,0,sizeof(ws_status));
	onion *o=websocket_server_new();
	onion_request *req=onion_request_new(onion_get_listen_point(o, 0));
	onion_request_write0(req,"GET /\n\n");
	onion_request_process(req);
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
	onion_request_process(req);
	FAIL_IF_NOT(ws_status.is_connected);
	FAIL_IF_NOT_EQUAL_INT(ws_status.connected,1);

	onion_request_free(req);
	onion_free(o);

	END_LOCAL();
}

void t03_websocket_server_receive_small_packet(){
	INIT_LOCAL();
	int length = 0;
	char *buffer = NULL, buffer2[115];
	memset(&ws_status,0,sizeof(ws_status));
	onion *o=websocket_server_new();
	onion_request *req=websocket_start_handshake(o);
	req->connection.listen_point->read = (lpreader_sig_t*)websocket_data_buffer_read;

	onion_response *res = onion_response_new(req);
	onion_websocket *ws = onion_websocket_new(req, res);

	length = websocket_forge_small_packet((char **)&buffer);
	websocket_data_buffer_write(req, buffer, length);

	onion_websocket_read(ws, (char *)&buffer2, 120);

	buffer2[114] = '\0';
	FAIL_IF_NOT_EQUAL_STR(buffer2, "Some UTF-8-encoded chars which will be cut at the 117th char so I write some gap-filling text with no meaning unti");

	onion_websocket_free(ws);
	onion_request_free(req);
	onion_free(o);
	END_LOCAL();
}

void t04_websocket_server_close_handshake(){
	INIT_LOCAL();
	int length = 0;
	unsigned char *buffer = NULL, buffer2[8];
	onion *o=websocket_server_new();
	onion_request *req=websocket_start_handshake(o);
	req->connection.listen_point->read = (lpreader_sig_t*) websocket_data_buffer_read;
	req->connection.listen_point->write = (lpwriter_sig_t*) websocket_data_buffer_write;

	onion_response *res = onion_response_new(req);
	onion_websocket *ws = onion_websocket_new(req, res);

	length = websocket_forge_close_packet((char **)&buffer);
	websocket_data_buffer_write(req, (char *)buffer, length);
	onion_connection_status ret = onion_websocket_call(ws);
	FAIL_IF(ret!=-2);
	websocket_data_buffer_read(req, (char *)&buffer2, 8);
	FAIL_IF_NOT(buffer2[0]==0x88);
	FAIL_IF_NOT(buffer2[1]==0x02);
	FAIL_IF_NOT(buffer2[2]==0x03);
	FAIL_IF_NOT(buffer2[3]==0xE8);

	onion_websocket_free(ws);
	onion_request_free(req);
	onion_free(o);
	END_LOCAL();
}

int main(int argc, char **argv){
	START();

	t01_websocket_server_no_ws();
	t02_websocket_server_w_ws();
	t03_websocket_server_receive_small_packet();
	t04_websocket_server_close_handshake();

	END();
}
