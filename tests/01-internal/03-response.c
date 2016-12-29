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

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <onion/low.h>
#include <onion/log.h>
#include <onion/dict.h>
#include <onion/request.h>
#include <onion/response.h>
#include <onion/types_internal.h>
#include <onion/onion.h>
#include <onion/http.h>

#include "../ctest.h"
#include "buffer_listen_point.h"

#define FILL(a,b) onion_request_write(a,b,strlen(b))


void t01_create_add_free(){
	INIT_LOCAL();

	onion_response *res;

	res=onion_response_new(NULL);
	FAIL_IF_NOT_EQUAL(res->code, 200);

	FAIL_IF_EQUAL(res,NULL);

	onion_response_set_length(res,1024);
	FAIL_IF_NOT_EQUAL_STR(onion_dict_get(res->headers,"Content-Length"),"1024");

	onion_response_free(res);

	END_LOCAL();
}



void t02_full_cycle_http10(){
	INIT_LOCAL();

	onion *server=onion_new(0);
	onion_add_listen_point(server,NULL,NULL,onion_buffer_listen_point_new());
	onion_request *request;
	char buffer[4096];
	memset(buffer,0,sizeof(buffer));

	request=onion_request_new(server->listen_points[0]);

	onion_response *response=onion_response_new(request);

	onion_response_set_length(response, 30);
	FAIL_IF_NOT_EQUAL(response->length,30);
	onion_response_write_headers(response);

	onion_response_write0(response,"123456789012345678901234567890");
	onion_response_flush(response);
	FAIL_IF_NOT_EQUAL(response->sent_bytes,30);

	onion_response_free(response);
	buffer[sizeof(buffer)-1]=0;
	strncpy(buffer,onion_buffer_listen_point_get_buffer_data(request),sizeof(buffer)-1);
	onion_request_free(request);
	onion_free(server);

	FAIL_IF_NOT_STRSTR(buffer, "HTTP/1.0 200 OK\r\n");
	FAIL_IF_NOT_STRSTR(buffer, "Connection: Keep-Alive\r\n");
	FAIL_IF_NOT_STRSTR(buffer, "Content-Length: 30\r\n");
	FAIL_IF_NOT_STRSTR(buffer, "Server: libonion");
	FAIL_IF_NOT_STRSTR(buffer, "coralbits");
	FAIL_IF_NOT_STRSTR(buffer, "\r\n\r\n123456789012345678901234567890");

	END_LOCAL();
}

void t03_full_cycle_http11(){
	INIT_LOCAL();

	onion *server=onion_new(0);
	onion_add_listen_point(server, NULL,NULL,onion_buffer_listen_point_new());
	onion_request *request;
	char buffer[4096];
	memset(buffer,0,sizeof(buffer));

	request=onion_request_new(server->listen_points[0]);
	FILL(request,"GET / HTTP/1.1\n");

	onion_response *response=onion_response_new(request);

	onion_response_set_length(response, 30);
	FAIL_IF_NOT_EQUAL(response->length,30);
	onion_response_write_headers(response);

	onion_response_write0(response,"123456789012345678901234567890");
	onion_response_flush(response);

	FAIL_IF_NOT_EQUAL(response->sent_bytes,30);

	onion_response_free(response);
	buffer[sizeof(buffer)-1]=0;
	strncpy(buffer,onion_buffer_listen_point_get_buffer_data(request),sizeof(buffer)-1);
	onion_request_free(request);
	onion_free(server);

	FAIL_IF_NOT_STRSTR(buffer, "HTTP/1.1 200 OK\r\n");
	FAIL_IF_STRSTR(buffer, "Connection: Keep-Alive\r\n");
	FAIL_IF_NOT_STRSTR(buffer, "Content-Length: 30\r\n");
	FAIL_IF_NOT_STRSTR(buffer, "Server: libonion");
	FAIL_IF_NOT_STRSTR(buffer, "coralbits");
	FAIL_IF_NOT_STRSTR(buffer, "\r\n\r\n123456789012345678901234567890");

	//FAIL_IF_NOT_EQUAL_STR(buffer, "HTTP/1.1 200 OK\r\nContent-Length: 30\r\nServer: libonion v0.1 - coralbits.com\r\n\r\n123456789012345678901234567890");

	END_LOCAL();
}

void t04_cookies(){
	INIT_LOCAL();

	onion_response *res=onion_response_new(NULL);
	onion_dict *h=onion_response_get_headers(res);
	bool ok;

	ok = onion_response_add_cookie(res, "key1", "value1", -1, NULL, NULL, 0);
	FAIL_IF_NOT_EQUAL_STR(onion_dict_get(h, "Set-Cookie"), "key1=value1");
	FAIL_IF_NOT_EQUAL(ok, true);

	onion_dict_remove(h, "Set-Cookie");
	ok = onion_response_add_cookie(res, "key2", "value2", -1, "/", "*.example.org", OC_HTTP_ONLY|OC_SECURE);
	FAIL_IF_NOT_EQUAL_STR(onion_dict_get(h, "Set-Cookie"), "key2=value2; path=/; domain=*.example.org; HttpOnly; Secure");
	FAIL_IF_NOT_EQUAL(ok, true);

	onion_dict_remove(h, "Set-Cookie");
	ok = onion_response_add_cookie(res, "key3", "value3", 0, "/", "*.example.org", OC_HTTP_ONLY|OC_SECURE);
	FAIL_IF_NOT_EQUAL_STR(onion_dict_get(h, "Set-Cookie"), "key3=value3; expires=Thu, 01 Jan 1970 00:00:00 GMT; path=/; domain=*.example.org; HttpOnly; Secure");
	FAIL_IF_NOT_EQUAL(ok, true);

	onion_dict_remove(h, "Set-Cookie");
	ok = onion_response_add_cookie(res, "key4", "value4", 60, "/", "*.example.org", OC_HTTP_ONLY|OC_SECURE);
	FAIL_IF_EQUAL_STR(onion_dict_get(h, "Set-Cookie"), "key4=value4; expires=Thu, 01 Jan 1970 00:00:00 GMT; path=/; domain=*.example.org; HttpOnly; Secure");
	FAIL_IF_EQUAL_STR(onion_dict_get(h, "Set-Cookie"), "key4=value4; domain=*.example.org; HttpOnly; path=/; Secure");
	FAIL_IF_NOT_EQUAL(ok, true);

	int i;
	int valid_expires=0;
	char tmpdate[100];
	const char *setcookie=onion_dict_get(h, "Set-Cookie");
	for(i=59;i<62;i++){
		struct tm *tmp;
		time_t t=time(NULL) + i;
		tmp = localtime(&t);
		strftime(tmpdate, sizeof(tmpdate), "key4=value4; expires=%a, %d %b %Y %H:%M:%S %Z; path=/; domain=*.example.org; HttpOnly; Secure", tmp);
		ONION_DEBUG("\ntest  %s =? \nonion %s", tmpdate, setcookie);
		if (strcmp(tmpdate, setcookie)==0)
			valid_expires=1;
	}
	FAIL_IF_NOT(valid_expires);

	// cookie too long
	onion_dict_remove(h, "Set-Cookie");
	const int long_value_len=1024*8;
	char *long_value=malloc(long_value_len);
	for(i=0;i<long_value_len;i++){
		long_value[i]='a';
	}
	long_value[long_value_len-1]=0;
	ok = onion_response_add_cookie(res, "key5", long_value, 60, "/", "*.example.org", OC_HTTP_ONLY|OC_SECURE);
	FAIL_IF_NOT_EQUAL(ok, false);
	FAIL_IF_NOT_EQUAL(onion_dict_get(h, "Set-Cookie"), NULL); // cookie not set

	free(long_value);

	onion_response_free(res);

	END_LOCAL();
}

void t05_printf(){
	INIT_LOCAL();

	onion *server=onion_new(0);
	onion_add_listen_point(server,NULL,NULL,onion_buffer_listen_point_new());
	onion_request *request;
	char buffer[4096];
	memset(buffer,0,sizeof(buffer));

	request=onion_request_new(server->listen_points[0]);

	onion_response *response=onion_response_new(request);

	onion_response_printf(response, "%s %d %p", "Hello world", 123, NULL);
	onion_response_flush(response);
	onion_response_free(response);
	buffer[sizeof(buffer)-1]=0;
	strncpy(buffer,onion_buffer_listen_point_get_buffer_data(request),sizeof(buffer)-1);
	onion_request_free(request);
	onion_free(server);

	FAIL_IF_NOT_STRSTR(buffer, "Hello world 123 (nil)");

	END_LOCAL();
}

void t06_empty(){
	INIT_LOCAL();

	onion *server=onion_new(0);
	onion_add_listen_point(server,NULL,NULL,onion_buffer_listen_point_new());
	onion_request *request;
	char buffer[4096];
	memset(buffer,0,sizeof(buffer));

	request=onion_request_new(server->listen_points[0]);
	onion_response *response=onion_response_new(request);

// 	onion_response_write_headers(response);

	onion_response_flush(response);
	onion_response_free(response);

	buffer[sizeof(buffer)-1]=0;
	strncpy(buffer,onion_buffer_listen_point_get_buffer_data(request),sizeof(buffer)-1);
	onion_request_free(request);
	onion_free(server);

	FAIL_IF_NOT_STRSTR(buffer, "Server:");
	FAIL_IF_NOT_STRSTR(buffer, "Content-Type:");

// 	ONION_DEBUG(buffer);

	END_LOCAL();
}

void t07_large_printf(){
	INIT_LOCAL();
	onion *server=onion_new(0);
	onion_add_listen_point(server,NULL,NULL,onion_buffer_listen_point_new());
	onion_request *request;
	char buffer[4096];
	memset(buffer,0,sizeof(buffer));

	request=onion_request_new(server->listen_points[0]);
	onion_response *response=onion_response_new(request);

	const size_t large_size=100000;
	char *large_buffer=onion_low_malloc(large_size);
	memset(large_buffer, 'a', large_size);
	large_buffer[large_size-1]=0;
	onion_response_printf(response, "%s", large_buffer);
	onion_low_free(large_buffer);

	onion_response_flush(response);
	onion_response_free(response);

	buffer[sizeof(buffer)-1]=0;
	strncpy(buffer,onion_buffer_listen_point_get_buffer_data(request),sizeof(buffer)-1);
	onion_request_free(request);
	onion_free(server);

	FAIL_IF_NOT_STRSTR(buffer, "Server:");
	FAIL_IF_NOT_STRSTR(buffer, "Content-Type:");


	END_LOCAL();
}

int main(int argc, char **argv){
	START();

	t01_create_add_free();
	t02_full_cycle_http10();
	t03_full_cycle_http11();
	t04_cookies();
	t05_printf();
	t06_empty();
	t07_large_printf();

	END();
}
