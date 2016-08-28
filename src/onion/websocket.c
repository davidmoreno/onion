/*
	Onion HTTP server library
	Copyright (C) 2010-2016 David Moreno Montero and others

	This library is free software; you can redistribute it and/or
	modify it under the terms of, at your choice:

	a. the Apache License Version 2.0.

	b. the GNU General Public License as published by the
		Free Software Foundation; either version 2.0 of the License,
		or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of both libraries, if not see
	<http://www.gnu.org/licenses/> and
	<http://www.apache.org/licenses/LICENSE-2.0>.
	*/

#include "log.h"
#include "websocket.h"
#include "response.h"
#include "types_internal.h"
#include "request.h"
#include "codecs.h"
#include "random.h"
#include "low.h"

#include <poll.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

/// @defgroup websocket Websockets. Basic websockets support.

/// @ingroup websocket
enum onion_websocket_flags_e{
	WS_FIN=1,
	WS_MASK=2,
};


// signature of writer in listen point.
/// @ingroup websocket
typedef ssize_t (lpwriter_sig_t)(onion_request *req, const char *data, size_t len);

// signature of writer in listen point.
/// @ingroup websocket
typedef ssize_t (lpreader_sig_t)(onion_request *req, char *data, size_t len);


static int onion_websocket_read_packet_header(onion_websocket *ws);

const static char *websocket_magic_13="258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
const static int websocket_magic_13_length=36;

/**
 * @short Creates a websocket connection from the given request to the response
 * @memberof onion_websocket_t
 * @ingroup websocket
 *
 * It makes all the checks about it being a proper websocket connection, and returns the
 * websocket object.
 *
 * Default opcode mode is OWS_TEXT.
 *
 * @param req The request
 * @param res The response
 * @returns The websocket if possible, or NULL if not possible.
 */
onion_websocket* onion_websocket_new(onion_request* req, onion_response *res)
{
	if (req->websocket)
		return req->websocket;

	onion_random_init();

	const char *upgrade=onion_request_get_header(req,"Upgrade");
	if (!upgrade || strcasecmp(upgrade,"websocket")!=0)
		return NULL;

	ONION_DEBUG("Websockets!");

	const char *ws_key=onion_request_get_header(req,"Sec-Websocket-Key");
	const char *ws_protocol=onion_request_get_header(req,"Sec-Websocket-Protocol");
	const char *ws_version=onion_request_get_header(req,"Sec-Websocket-Version");

	if (!ws_version || !ws_key ){
		ONION_ERROR("Websocket is missing some data: need key and version");
		return NULL;
	}
	if (atoi(ws_version)!=13){
		ONION_ERROR("Websocket version not supported. Currenty only support version 13, asked %s", ws_version);
		return NULL;
	}

	int length=strlen(ws_key);
	char *tmp=alloca(length+websocket_magic_13_length+1);
	strcpy(tmp,ws_key);
	strcat(tmp,websocket_magic_13);
	tmp[length+websocket_magic_13_length]=0;

	char ws_sha1[20];
	onion_sha1(tmp, length+websocket_magic_13_length, ws_sha1);

	char *key_answer=onion_base64_encode(ws_sha1, 20);
	char *p;
	for(p=key_answer;*p!='\n';++p);
	*p=0;

	onion_response_set_code(res, HTTP_SWITCH_PROTOCOL);
	res->flags|=OR_CONNECTION_UPGRADE;
	res->flags|=OR_LENGTH_SET; // Fix for Chrome to close cleanly websocket connections
	onion_response_set_header(res, "Upgrade", "websocket");
	if (ws_protocol)
		onion_response_set_header(res, "Sec-Websocket-Procotol", ws_protocol);
	onion_response_set_header(res, "Sec-Websocket-Accept", key_answer);
	onion_low_free(key_answer);

	onion_response_write_headers(res);
	onion_response_write(res, "",0); // AKA flush

	onion_websocket *ret=onion_low_malloc(sizeof(onion_websocket));
	ret->callback=NULL;
	ret->req=req;
	ret->data_left=0;
	ret->flags=0;
	ret->user_data=req->data;
	ret->free_user_data=NULL;
	ret->opcode=OWS_TEXT;

	req->websocket=ret;

	return ret;
}

/**
 * @short Frees the websocket
 * @memberof onion_websocket_t
 * @ingroup websocket
 *
 * Frees the websocket, and if possible, the userdata. There is no need to call it as the request does it when the
 * request finishes.
 *
 * @param ws
 */
void onion_websocket_free(onion_websocket *ws){
	if(ws->callback)
		ws->callback(ws->user_data, ws, -1);

	if (ws->free_user_data)
		ws->free_user_data(ws->user_data);

	onion_random_free();

	ws->req->websocket=NULL; // To avoid double free on stupid programs that call this directly.
	onion_low_free(ws);
}


/**
 * @short Writes a fragment to the websocket
 * @memberof onion_websocket_t
 * @ingroup websocket
 *
 * @param ws The Websocket
 * @param buffer Data to write
 * @param _len Length of data to write
 * @returns Bytes written or <0 if error writting.
 */
int onion_websocket_write(onion_websocket* ws, const char* buffer, size_t _len)
{
        if (!ws->req) { // should not happen....
	  ONION_DEBUG ("no request in websocket@%p", ws);
	  return -1;
	}
        lpwriter_sig_t* lpwriter = ws->req->connection.listen_point->write;
	ssize_t len=_len; // I need it signed here
	if (!lpwriter) {
	  ONION_DEBUG ("no listen point writer for websocket@%p", ws);
	  return -1;
	}
	//ONION_DEBUG("Write %d bytes",len);
	{
		char header[16];
		int hlen=2;
		header[0]=0x80|(ws->opcode&0x0F); // Also final in fragment.
		header[1]=0x00; // Do not mask on send
		if (len<126)
			header[1]|=len;
		else if (len<0x0FFFF){
			header[1]|=126;
			header[2]=(len>>8)&0x0FF;
			header[3]=(len)&0x0FF;
			hlen+=2;
		}
		else{
			header[1]|=127;
			int i;
			int tlen=len;
			for(i=0;i<8;i++){
				header[3+8-i]=tlen&0x0FF;
				tlen>>=8;
			}
			hlen+=8;
		}

		/*
		int i;
		for(i=0;i<hlen;i++){
			ONION_DEBUG("%d: %02X", i, header[i]&0x0FF);
		}
		*/

		(*lpwriter) (ws->req, header, hlen);
	}
	/// FIXME! powerup using int32_t as basic type.
	int ret=0;
	int i,j;
	char tout[1024];
	for(i=0;i<len-1024;i+=1024){
		for (j=0;j<sizeof(tout);j++){
			//ONION_DEBUG("At %d, %02X ^ %02X = %02X", (i+j)&1023, buffer[i+j]&0x0FF, mask[j&3]&0x0FF, (buffer[i]^mask[j&3])&0x0FF);
			tout[j]=buffer[i+j];
		}
		ret+= (*lpwriter) (ws->req, tout, sizeof(tout));
	}
	for(;i<len;i++){
		//ONION_DEBUG("At %d, %02X ^ %02X = %02X", i&1023, buffer[i]&0x0FF, mask[i&3]&0x0FF, (buffer[i]^mask[i&3])&0x0FF);
		tout[i&1023]=buffer[i];
	}

	return (*lpwriter) (ws->req, tout, len&1023);
}

/**
 * @short Reads some data from the websocket.
 * @memberof onion_websocket_t
 * @ingroup websocket
 *
 * It attemps to read len bytes from the websocket. If there is enought data it reads it from the
 * current fragment, but it may block waiting for next fragment if data is exhausted.
 *
 * Check onion_websocket_set_callback for more info on how to use websockets.
 *
 * @param ws Wbsocket object
 * @param buffer Data to write
 * @param len Length of buffer to write
 * @returns number of bytes written or <0 if error.
 */
int onion_websocket_read(onion_websocket* ws, char* buffer, size_t len)
{
        if (!ws->req) {
          ONION_DEBUG("no request in websocket@%p", ws);
	  return -1;
	}
        lpreader_sig_t* lpreader = ws->req->connection.listen_point->read;
	if (!lpreader) {
	  ONION_DEBUG("no listen point reader in websocket@%p", ws);
	  return -1;
	}
	//ONION_DEBUG("Please, read %d bytes, %d ready", len, ws->data_left);
	if (ws->data_left==0){
		onion_connection_status status;
		if ((status = onion_websocket_read_packet_header(ws))<0){
			if(status == -2)
				return -2;
			ONION_ERROR("Error reading websocket header (%i)", status);
			return -1;
		}
	}

	size_t left_len=0;
	if (len>ws->data_left){
		left_len=len-ws->data_left;
		len=ws->data_left;
		//ONION_DEBUG("Read %d bytes now, %d bytes later", len, left_len);
	}
	int r= (*lpreader) (ws->req, buffer, len);
	if (ws->flags&WS_MASK){
		char *p=buffer;
		int i;
		for(i=0;i<r;i++){
			*p=*p^ws->mask[(ws->mask_pos++)&3];
			p++;
		}
	}
	ws->data_left-=r;

	if (left_len && r==len)
		r+=onion_websocket_read(ws, &buffer[len], left_len);

	if (r<0 || (r==0 && errno!=0)){
		ws->callback=NULL; // Easy way to force close of websocket, which is ok as it closed.
	}
	return r;
}


 /**
  * @short Uses vprintf-style writing to the websocket
  * @memberof onion_websocket_t
	* @ingroup websocket
  *
  * It writes the message as a single fragment. Above 512 bytes, a
  * scalar buffer is temporarily malloc-ed then free-d ...
  *
  * @param ws The websocket
  * @param fmt printf-like format string, and args containing the arguments
  * @returns Number of bytes written
  */
 int onion_websocket_vprintf(onion_websocket* ws, const char* fmt, va_list args)
 {
   char temp[512];
   int l=vsnprintf(temp, sizeof(temp)-1, fmt, args);
   if (l < sizeof(temp))
     return onion_websocket_write(ws, temp, l);
   else {
     ssize_t s;
     char*buf = onion_low_scalar_malloc(l+1);
     if (!buf) {
       // this cannot happen, since onion_low_scalar_malloc
       // handles that error...
       ONION_ERROR("Could not reserve %d bytes", l+1);
       return -1;
     }
     vsnprintf(buf, l, fmt, args);
     s = onion_websocket_write (ws, buf, l);
     onion_low_free (buf);
     return s;
   }
 }


/**
 * @short Uses printf-style writing to the websocket
 * @memberof onion_websocket_t
 * @ingroup websocket
 *
 * It writes the message as a single fragment, using onion_websocket_vprintf ...
 *
 * @param ws The websocket
 * @param fmt printf-like format string, and next parameters are the arguments
 * @returns Number of bytes written
 */
int onion_websocket_printf(onion_websocket* ws, const char* fmt, ... )
{
	va_list ap;
	va_start(ap, fmt);
	int l= onion_websocket_vprintf(ws, fmt, ap);
	va_end(ap);
	return l;
}

/**
 * @short Sets the next callback to call if more data is available.
 * @memberof onion_websocket_t
 * @ingroup websocket
 *
 * Websockets are callback/blocking based. Users of websockets can choose which option to use, as required by the application.
 *
 * Both are not incompatible.
 *
 * The blocking mode just tries to read more data than available, so it will block until some data is ready.
 *
 * The non-blocking asynchronous mode sets a callback to call when data is ready, and it passes this callback the websocket, some
 * internal data, and the lenght of data ready to be read. In this callback the callback can be changed, so that next calls will
 * call that callback. If not all data is used, it will be called inmediatly on exit.
 *
 * @param ws Websocket
 * @param cb The callback function to call: onion_connection_status callback_signature(void *data, onion_websocket *ws, size_t data_ready_len);
 */
void onion_websocket_set_callback(onion_websocket* ws, onion_websocket_callback_t cb)
{
	ws->callback=cb;
}

/**
 * @short Sets the userdata to use for this websocket connection.
 * @memberof onion_websocket_t
 * @ingroup websocket
 *
 * By default it will contain the request user data, but can be changed to any.
 *
 * If a free user data funtion is provided it will be called to free old userdatas when
 * a new one is set, or when websocket is destroyed.
 *
 * This userdata can be used to store connection session data between callback calls.
 *
 * @param ws Websocket connection
 * @param userdata User data
 * @param free_userdata Function to call to free user data.
 */
void onion_websocket_set_userdata(onion_websocket *ws, void *userdata, void (*free_userdata)(void *)){
	if (ws->free_user_data)
		ws->free_user_data(ws->user_data);
	ws->user_data=userdata;
	ws->free_user_data=free_userdata;
}

/**
 * @short Reads a packet header.
 */
static onion_connection_status onion_websocket_read_packet_header(onion_websocket *ws){
	char tmp[8];
	unsigned char *utmp=(unsigned char*)tmp;
	if (!ws->req) { // should not happen....
	  ONION_DEBUG("no request in websocket@%p", ws);
	  return OCS_CLOSE_CONNECTION;
	}
	lpreader_sig_t* lpreader = ws->req->connection.listen_point->read;
	if (!lpreader) {
	  ONION_DEBUG("no listen point reader in websocket@%p", ws);
	  return OCS_CLOSE_CONNECTION;
	}
	int r= (*lpreader) (ws->req, tmp, 2);
	//ONION_DEBUG("reading input r = %i", r);
	if (r!=2){ ONION_DEBUG("Error reading header"); return OCS_CLOSE_CONNECTION; }

	ws->flags=0;
	if (tmp[0]&0x80)
		ws->flags|=WS_FIN;
	if (tmp[1]&0x80)
		ws->flags|=WS_MASK;
	ws->opcode=tmp[0]&0x0F;
	ws->data_left=tmp[1]&0x7F;
	if (ws->data_left==126){
	  r= (*lpreader) (ws->req, tmp, 2);
		if (r!=2){ ONION_DEBUG("Error reading header"); return OCS_CLOSE_CONNECTION; }
		ONION_DEBUG("%d %d", utmp[0], utmp[1]);
		ws->data_left=utmp[1] + utmp[0]*256;
	}
	else if (ws->data_left==127){
	  r= (*lpreader) (ws->req, tmp, 8);
		//ONION_DEBUG("%d %d %d %d %d %d %d", utmp[0], utmp[1], utmp[2], utmp[3], utmp[4], utmp[5], utmp[6], utmp[7]);
		if (r!=8){ ONION_DEBUG("Error reading header"); return OCS_CLOSE_CONNECTION; }
		ws->data_left=0;
		int i;
		for(i=0;i<8;i++)
			ws->data_left+=((uint64_t)utmp[i])<<((7-i)*8); // Maybe problematic on 32bits systems
	}
	ONION_DEBUG("Data left %d", ws->data_left);
	if (ws->flags&WS_MASK){
	        r= (*lpreader) (ws->req, ws->mask, 4);
		//ONION_DEBUG("bytes read=%i", r);
		if (r!=4){ ONION_DEBUG("Error reading header (4)"); return OCS_CLOSE_CONNECTION; }
		ws->mask_pos=0;
	}

	if (ws->opcode==OWS_PING){ // I do answer ping myself.
		onion_websocket_set_opcode(ws, OWS_PONG);
		char *data=onion_low_scalar_malloc(ws->data_left);
		ssize_t r=onion_websocket_read(ws,data, ws->data_left);

		onion_websocket_write(ws, data, r);
	}
	if (ws->opcode==OWS_CONNECTION_CLOSE){ // Closing connection
		r = (*lpreader) (ws->req, tmp, 2);
		if(r!=2){ ONION_DEBUG("Error reading status code"); return OCS_CLOSE_CONNECTION; }
		char status[2];
		status[0] = utmp[0] ^ ws->mask[0];
		status[1] = utmp[1] ^ ws->mask[1];
		ONION_DEBUG("Connection closed by client, status=%u", (status[0]<<8) + status[1]);
		onion_websocket_close(ws, (const char *)&status);
		return OCS_CLOSE_CONNECTION;
	}
	return OCS_NEED_MORE_DATA;
	//ONION_DEBUG("Mask %02X %02X %02X %02X", ws->mask[0]&0x0FF, ws->mask[1]&0x0FF, ws->mask[2]&0x0FF, ws->mask[3]&0x0FF);
}

/**
 * @short Used internally when new data is ready on the websocket file descriptor.
 * @memberof onion_websocket_t
 * @ingroup websocket
 */
onion_connection_status onion_websocket_call(onion_websocket* ws)
{
	onion_connection_status ret=OCS_NEED_MORE_DATA;
	while(ret==OCS_NEED_MORE_DATA){
		if (ws->req->connection.fd>0){
			struct pollfd pfd;
			pfd.events=POLLIN;
			pfd.fd=ws->req->connection.fd;
			//ONION_DEBUG("Wait for data");
			int r=poll(&pfd,1, -1);
			if (r==0)
				return OCS_INTERNAL_ERROR;
			//ONION_DEBUG("waited for data fd %d -- res %d -- events %d", ws->req->fd, r, pfd.events);
		}
		else
			sleep(1); // FIXME Worst possible solution. But solution anyway to the problem of not know when new data is available.
		if (ws->callback){
			//ONION_DEBUG("data left %i", ws->data_left);
			if (ws->data_left==0){
				onion_connection_status err=onion_websocket_read_packet_header(ws);
				if (err<0){
					if(err != -2) ONION_ERROR("Error reading websocket header (%i)", err);
					return err;
				}
			}

			size_t last_d_l;
			do{
				last_d_l=ws->data_left;
				ret=ws->callback(ws->user_data, ws, ws->data_left);
			}while(ws->data_left!=0 && last_d_l!=ws->data_left && ws->callback);
		}

		if (!ws->callback) // ws->callback can change ws->callback, so another test, not else.
			ret=OCS_CLOSE_CONNECTION;
	}
	ONION_DEBUG("Websocket connection closed (%d)", ret);
	if (ret==OCS_CLOSE_CONNECTION)
		return ret;
	return OCS_INTERNAL_ERROR;
}

/**
 * @short Closes the websocket sending the close opcode (8)
 */
void onion_websocket_close(onion_websocket *ws, const char *status){
	onion_websocket_set_opcode(ws, OWS_CONNECTION_CLOSE);
	onion_websocket_write(ws, status, 2);
}


/**
 * @short Sets the opcode for the websocket
 * @ingroup websocket
 *
 * This can be called before writes to change the meaning of the write.
 *
 * WARNING: If untouched, its the opcode of the last received fragment.
 *
 * This is the normal desired way, as if server talks to you in text normally you answer in text,
 * and so on.
 *
 * @param ws The websocket
 * @param opcode The new opcode, from onion_websocket_opcode enum (matches specification numbers)
 */
void onion_websocket_set_opcode(onion_websocket *ws, onion_websocket_opcode opcode){
	ws->opcode=opcode;
}

/**
 * @short Returns current in use opcodes
 * @ingroup websocket
 *
 * @param ws The websocket
 * @returns The current opcodes in use
 */
onion_websocket_opcode onion_websocket_get_opcode(onion_websocket* ws)
{
	return ws->opcode;
}
