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
#include <netdb.h>

#include <onion/onion.h>
#include <onion/dict.h>
#include <onion/request.h>
#include <onion/types_internal.h>
#include <onion/listen_point.h>
#include <onion/http.h>

#include "../ctest.h"
#include "buffer_listen_point.h"

onion *server;
onion_listen_point *custom_io;

void setup(){
	server=onion_new(O_ONE);
	custom_io=onion_buffer_listen_point_new();
	onion_add_listen_point(server, NULL, NULL, custom_io);
}

#define REQ_WRITE(req, txt) onion_request_write(req, txt, strlen(txt));

void teardown(){
	onion_free(server);
}

void t01_create_add_free(){
	INIT_LOCAL();

	onion_request *req;
	int ok;

	req=onion_request_new(custom_io);

	FAIL_IF_EQUAL(req,NULL);

	ok=REQ_WRITE(req, "GET / HTTP/1.1\n");
	FAIL_IF_NOT(ok);

	onion_request_free(req);

	END_LOCAL();
}

void t02_create_add_free_overflow(){
	INIT_LOCAL();

	onion_request *req;
	int ok, i;

	req=onion_request_new(custom_io);
	FAIL_IF_NOT_EQUAL(req->connection.fd, -1);

	FAIL_IF_EQUAL(req,NULL);

	char of[14096];
	for (i=0;i<sizeof(of);i++)
		of[i]='a'+i%26;
	of[i-1]='\0';
	char get[14096*4];

	sprintf(get,"%s %s %s",of,of,of);
	ok=REQ_WRITE(req,get);
	FAIL_IF_NOT_EQUAL(ok,OCS_INTERNAL_ERROR);
	onion_request_clean(req);

	sprintf(get,"%s %s %s\n",of,of,of);
	ok=REQ_WRITE(req,get);
	FAIL_IF_NOT_EQUAL(ok,OCS_INTERNAL_ERROR);
	onion_request_clean(req);


	sprintf(get,"GET %s %s\n",of,of);
	ok=REQ_WRITE(req,get);
	printf("%d\n",ok);
	FAIL_IF_NOT_EQUAL(ok,OCS_INTERNAL_ERROR);

	onion_request_free(req);

	END_LOCAL();
}

void t03_create_add_free_full_flow(){
	INIT_LOCAL();

	onion_request *req;
	int ok;

	req=onion_request_new(custom_io);
	FAIL_IF_EQUAL(req,NULL);
	FAIL_IF_NOT_EQUAL(req->connection.fd, -1);

	ok=REQ_WRITE(req,"GET /myurl%20/is/very/deeply/nested?test=test&query2=query%202&more_query=%20more%20query+10&empty&empty2 HTTP/1.0\n");
	FAIL_IF_NOT(ok);
	ok=REQ_WRITE(req,"Host: 127.0.0.1\r\n");
	FAIL_IF_NOT(ok);
	ok=REQ_WRITE(req,"Other-Header: My header is very long and with spaces...\n");
	FAIL_IF_NOT(ok);
	ok=REQ_WRITE(req,"Final-Header: This header do not get into headers as a result of now knowing if its finished, or if its multiline.\n");
	FAIL_IF_NOT(ok);

	FAIL_IF_EQUAL(req->flags,OR_GET|OR_HTTP11);

	FAIL_IF_EQUAL(req->headers, NULL);
	FAIL_IF_NOT_EQUAL_STR( onion_dict_get(req->headers,"Host"), "127.0.0.1");
	FAIL_IF_NOT_EQUAL_STR( onion_dict_get(req->headers,"Other-Header"), "My header is very long and with spaces...");

	FAIL_IF_NOT_EQUAL_STR(req->fullpath,"/myurl /is/very/deeply/nested");
  FAIL_IF_NOT_EQUAL(req->path,NULL);
  onion_request_process(req); // this should set the req->path.
	FAIL_IF_NOT_EQUAL_STR(req->path,"myurl /is/very/deeply/nested");

	FAIL_IF_EQUAL(req->GET, NULL);
	FAIL_IF_NOT_EQUAL_STR( onion_dict_get(req->GET,"test"), "test");
	FAIL_IF_NOT_EQUAL_STR( onion_dict_get(req->GET,"query2"), "query 2");
	FAIL_IF_NOT_EQUAL_STR( onion_dict_get(req->GET,"more_query"), " more query 10");
	FAIL_IF_EQUAL(onion_request_get_query(req, "empty"), NULL);
	FAIL_IF_EQUAL(onion_request_get_query(req, "empty2"), NULL);
	FAIL_IF_NOT_EQUAL(onion_request_get_query(req, "empty3"), NULL);

	onion_request_free(req);

	END_LOCAL();
}

void t04_create_add_free_GET(){
	INIT_LOCAL();

	onion_request *req;
	int ok;

	req=onion_request_new(custom_io);
	FAIL_IF_EQUAL(req,NULL);
	FAIL_IF_NOT_EQUAL(req->connection.fd, -1);

	const char *query="GET /myurl%20/is/very/deeply/nested?test=test&query2=query%202&more_query=%20more%20query+10 HTTP/1.0\n"
													"Host: 127.0.0.1\n\r"
													"Other-Header: My header is very long and with spaces...\r\n\r\n";

	int i; // Straight write, with clean (keep alive like)
	for (i=0;i<10;i++){
		FAIL_IF_NOT_EQUAL_INT(req->flags,0);
		ok=REQ_WRITE(req, query);
		FAIL_IF_NOT_EQUAL_INT(ok, OCS_REQUEST_READY);
		FAIL_IF_EQUAL(req->flags,OR_GET|OR_HTTP11);

		FAIL_IF_EQUAL(req->headers, NULL);
		FAIL_IF_NOT_EQUAL_STR( onion_dict_get(req->headers,"Host"), "127.0.0.1");
		FAIL_IF_NOT_EQUAL_STR( onion_dict_get(req->headers,"Other-Header"), "My header is very long and with spaces...");
    FAIL_IF_NOT_EQUAL_STR( onion_dict_get(req->headers,"other-heaDER"), "My header is very long and with spaces...");
    FAIL_IF_NOT_EQUAL_STR( onion_request_get_header(req,"other-heaDER"), "My header is very long and with spaces...");

		FAIL_IF_NOT_EQUAL_STR(req->fullpath,"/myurl /is/very/deeply/nested");
		FAIL_IF_NOT_EQUAL(req->path,NULL);
		onion_request_polish(req);
		FAIL_IF_NOT_EQUAL_STR(req->path,"myurl /is/very/deeply/nested");

		FAIL_IF_EQUAL(req->GET,NULL);
		FAIL_IF_NOT_EQUAL_STR( onion_dict_get(req->GET,"test"), "test");
		FAIL_IF_NOT_EQUAL_STR( onion_dict_get(req->GET,"query2"), "query 2");
		FAIL_IF_NOT_EQUAL_STR( onion_dict_get(req->GET,"more_query"), " more query 10");

		onion_request_clean(req);
		FAIL_IF_NOT_EQUAL(req->GET,NULL);
	}

	onion_request_free(req);

	END_LOCAL();
}

void t05_create_add_free_POST(){
	INIT_LOCAL();

	onion_request *req;
	int ok;

	req=onion_request_new(custom_io);
	FAIL_IF_EQUAL(req,NULL);
	FAIL_IF_NOT_EQUAL(req->connection.fd, -1);

	const char *query="POST /myurl%20/is/very/deeply/nested?test=test&query2=query%202&more_query=%20more%20query+10 HTTP/1.0\n"
													"Host: 127.0.0.1\n\rContent-Length: 50\nContent-Type: application/x-www-form-urlencoded\n"
													"Other-Header: My header is very long and with spaces...\r\n\r\nempty_post=&post_data=1&post_data2=2&empty_post_2=\n";

	int i; // Straight write, with clean (keep alive like)
	for (i=0;i<10;i++){
		FAIL_IF_NOT_EQUAL(req->flags,0);
		ok=REQ_WRITE(req,query);

		FAIL_IF_NOT_EQUAL(ok, OCS_REQUEST_READY);
		FAIL_IF_EQUAL(req->flags,OR_GET|OR_HTTP11);

		FAIL_IF_EQUAL(req->headers, NULL);
		FAIL_IF_NOT_EQUAL_STR( onion_dict_get(req->headers,"Host"), "127.0.0.1");
		FAIL_IF_NOT_EQUAL_STR( onion_dict_get(req->headers,"Other-Header"), "My header is very long and with spaces...");

		FAIL_IF_NOT_EQUAL_STR(req->fullpath,"/myurl /is/very/deeply/nested");
		FAIL_IF_NOT_EQUAL(req->path,NULL);
		onion_request_polish(req);
		FAIL_IF_NOT_EQUAL_STR(req->path,"myurl /is/very/deeply/nested");

		FAIL_IF_EQUAL(req->GET,NULL);
		FAIL_IF_NOT_EQUAL_STR( onion_dict_get(req->GET,"test"), "test");
		FAIL_IF_NOT_EQUAL_STR( onion_dict_get(req->GET,"query2"), "query 2");
		FAIL_IF_NOT_EQUAL_STR( onion_dict_get(req->GET,"more_query"), " more query 10");

		const onion_dict *post=onion_request_get_post_dict(req);
		FAIL_IF_EQUAL(post,NULL);
		FAIL_IF_NOT_EQUAL_STR( onion_dict_get(post,"post_data"), "1");
		FAIL_IF_NOT_EQUAL_STR( onion_dict_get(post,"post_data2"), "2");
		FAIL_IF_NOT_EQUAL_STR( onion_request_get_post(req, "empty_post"), "");
		FAIL_IF_NOT_EQUAL_STR( onion_request_get_post(req, "empty_post_2"), "");

		onion_request_clean(req);
		FAIL_IF_NOT_EQUAL(req->GET,NULL);
	}

	onion_request_free(req);

	END_LOCAL();
}

void t06_create_add_free_bad_method(){
	INIT_LOCAL();

	onion_request *req;
	int ok;

	req=onion_request_new(custom_io);
	//FAIL_IF_NOT_EQUAL(req->socket, (void*)111);

	FAIL_IF_EQUAL(req,NULL);

	ok=REQ_WRITE(req,"XGETX / HTTP/1.1\n");
	FAIL_IF_NOT_EQUAL(ok,OCS_NOT_IMPLEMENTED);

	onion_request_free(req);

	END_LOCAL();
}

void t06_create_add_free_POST_toobig(){
	INIT_LOCAL();

	onion_request *req;
	int ok;

	req=onion_request_new(custom_io);
	FAIL_IF_EQUAL(req,NULL);
	FAIL_IF_NOT_EQUAL(req->connection.fd, -1);

	onion_set_max_post_size(server, 10); // Very small limit

	const char *query="POST /myurl%20/is/very/deeply/nested?test=test&query2=query%202&more_query=%20more%20query+10 HTTP/1.0\n"
													"Host: 127.0.0.1\n\rContent-Length: 24\n"
													"Other-Header: My header is very long and with spaces...\r\n\r\npost_data=1&post_data2=2&post_data=1&post_data2=2";

	int i; // Straight write, with clean (keep alive like)
	for (i=0;i<10;i++){
		FAIL_IF_NOT_EQUAL(req->flags,0);
		ok=REQ_WRITE(req,query);

		FAIL_IF_NOT_EQUAL(ok,OCS_INTERNAL_ERROR);

		onion_request_clean(req);
		FAIL_IF_NOT_EQUAL(req->GET,NULL);
	}

	onion_request_free(req);

	END_LOCAL();
}


void t07_multiline_headers(){
	INIT_LOCAL();

	onion_request *req;
	int ok;

	onion_set_max_post_size(server, 1024);
	req=onion_request_new(custom_io);
	FAIL_IF_EQUAL(req,NULL);
	FAIL_IF_NOT_EQUAL(req->connection.fd, -1);

	{
		const char *query="GET / HTTP/1.0\n"
											"Host: 127.0.0.1\n\rContent-Length: 24\n"
											"Other-Header: My header is very long and with several\n lines\n"
											"Extra-Other-Header: My header is very long and with several\n \n lines\n"
											"My-Other-Header: My header is very long and with several\n\tlines\n\n";

		ok=onion_request_write(req,query,strlen(query));
	}
	FAIL_IF_EQUAL(ok,OCS_INTERNAL_ERROR);
	FAIL_IF_NOT_EQUAL_STR(onion_request_get_header(req,"other-header"),"My header is very long and with several lines");
	FAIL_IF_NOT_EQUAL_STR(onion_request_get_header(req,"extra-other-header"),"My header is very long and with several lines");
	FAIL_IF_NOT_EQUAL_STR(onion_request_get_header(req,"My-other-header"),"My header is very long and with several lines");
	onion_request_clean(req);

	{
		const char *query="GET / HTTP/1.0\n"
											"Host: 127.0.0.1\n\rContent-Length: 24\n"
											"Other-Header: My header is very long and with several\n lines\n"
											"My-Other-Header: My header is very long and with several\nlines\n\n";

		ok=onion_request_write(req,query,strlen(query));
	}
	FAIL_IF_NOT_EQUAL(ok,OCS_INTERNAL_ERROR); // No \t at my-other-header

	onion_request_free(req);


	END_LOCAL();
}

void t08_sockaddr_storage(){
	INIT_LOCAL();
	onion_request *req;


	{
		struct sockaddr_storage client_addr;
		socklen_t client_len=0;

		req=onion_request_new(custom_io);
		FAIL_IF_EQUAL(onion_request_get_sockadd_storage(req, NULL), &client_addr);
		FAIL_IF_EQUAL(onion_request_get_sockadd_storage(req, &client_len), &client_addr);
		FAIL_IF_NOT_EQUAL_INT(client_len, 0);
		onion_request_free(req);
	}

	{
		struct sockaddr_storage *client_addr;
		socklen_t client_len=0;
		struct addrinfo hints;
		struct addrinfo *result, *rp;
		char hostA[128], portA[16];
		char hostB[128], portB[16];

		memset(&hints,0, sizeof(struct addrinfo));
		hints.ai_canonname=NULL;
		hints.ai_addr=NULL;
		hints.ai_next=NULL;
		hints.ai_socktype=SOCK_STREAM;
		hints.ai_family=AF_UNSPEC;
		hints.ai_flags=AI_PASSIVE|AI_NUMERICSERV;

		int err=getaddrinfo("localhost","8080", &hints, &result);
		FAIL_IF_NOT_EQUAL_INT(err,0);
		if (err!=0)
			goto exit;
		for(rp=result;rp!=NULL;rp=rp->ai_next){
			memset(hostA,0,sizeof(hostA));
			memset(hostB,0,sizeof(hostB));
			memset(portA,0,sizeof(portA));
			memset(portB,0,sizeof(portB));

			getnameinfo(rp->ai_addr, rp->ai_addrlen, hostA, sizeof(hostA), portA, sizeof(portA), NI_NUMERICHOST | NI_NUMERICSERV);
			req=onion_request_new_from_socket(NULL, 0,(struct sockaddr_storage *)rp->ai_addr, rp->ai_addrlen);
			client_addr=onion_request_get_sockadd_storage(req, &client_len);
			FAIL_IF_EQUAL(client_addr, (struct sockaddr_storage *)rp->ai_addr);
			FAIL_IF_EQUAL(client_addr, NULL);

			getnameinfo((struct sockaddr *)client_addr, client_len, hostB, sizeof(hostB), portB, sizeof(portB), NI_NUMERICHOST | NI_NUMERICSERV);

			FAIL_IF_NOT_EQUAL_STR(hostA, hostB);
			FAIL_IF_NOT_EQUAL_STR(portA, portB);
			FAIL_IF_NOT_EQUAL_STR(hostA, onion_request_get_client_description(req));
			onion_request_free(req);
		}
		freeaddrinfo(result);

	}

	{
		req=onion_request_new(custom_io); //NULL, NULL, NULL);
		struct sockaddr_storage *client_addr;
		socklen_t client_len;
		client_addr=onion_request_get_sockadd_storage(req, &client_len);
		FAIL_IF_NOT_EQUAL(client_addr, NULL);
		onion_request_free(req);
	}

exit:
	END_LOCAL();
}

void t09_very_long_header(){
	INIT_LOCAL();

	onion_request *req;
	int ok;

	onion_set_max_post_size(server, 1024);
	req=onion_request_new(custom_io);
	FAIL_IF_EQUAL(req,NULL);
	FAIL_IF_NOT_EQUAL(req->connection.fd, -1);

	{
		const char *query_t="GET / HTTP/1.0\n"
											"Content-Type: application/x-www-form-urlencoded\n"
											"Host: 127.0.0.1\n\r"
											"Content-Length: 24\n"
											"Accept-Language: en\n"
											"Content-Type: ";
		const int longsize=strlen(query_t)+16;
		char *query=malloc(longsize); // 1MB enought?
		strcpy(query, query_t);
		int i;
		for (i=strlen(query); i<longsize -2; i++) // fill with crap
			query[i]='a'+(i%30);
		query[longsize-3]='\n';
		query[longsize-2]='\n';
		query[longsize-1]=0;

		FAIL_IF_NOT_EQUAL_INT(strlen(query), longsize-1);

		ok=onion_request_write(req, query, longsize);
		free(query);
	}
	FAIL_IF_EQUAL_INT(ok,OCS_INTERNAL_ERROR);

	onion_request_free(req);

	END_LOCAL();
}

void t10_repeated_header(){
	INIT_LOCAL();

	onion_request *req;
	int ok;

	onion_set_max_post_size(server, 1024);
	req=onion_request_new(custom_io);
	FAIL_IF_EQUAL(req,NULL);
	FAIL_IF_NOT_EQUAL(req->connection.fd, -1);

	{
		const char *query="GET / HTTP/1.0\n"
											"Content-Type: application/x-www-form-urlencoded\n"
											"Host: 127.0.0.1\n\r"
											"Content-Length: 24\n"
											"Accept-Language: en\n"
											"Content-Type: application/x-www-form-urlencoded-bis\n\n";

		ok=onion_request_write(req,query,strlen(query));
	}
	FAIL_IF_EQUAL(ok,OCS_INTERNAL_ERROR);
	FAIL_IF_NOT_EQUAL_STR(onion_request_get_header(req,"Host"),"127.0.0.1");
	FAIL_IF_NOT_EQUAL_STR(onion_request_get_header(req,"Content-Length"),"24");
	FAIL_IF_NOT_EQUAL_STR(onion_request_get_header(req,"Content-Type"),"application/x-www-form-urlencoded");


	onion_request_free(req);


	END_LOCAL();
}

void t11_cookies(){
	INIT_LOCAL();

	onion_request *req;
	int ok;

	req=onion_request_new(custom_io);
	FAIL_IF_EQUAL(req,NULL);
	FAIL_IF_NOT_EQUAL(req->connection.fd, -1);

	{
		const char *query="GET / HTTP/1.0\n"
											"Content-Type: application/x-www-form-urlencoded\n"
											"Host: 127.0.0.1\n\r"
											"Cookie: key1=value1; key2=value2;\n"
											"Accept-Language: en\n"; // Missing \n caused memleak, to check with valgrind

		ok=onion_request_write(req,query,strlen(query));
	}
	FAIL_IF_EQUAL(ok,OCS_INTERNAL_ERROR);
	FAIL_IF_NOT_EQUAL_STR(onion_request_get_header(req,"Host"),"127.0.0.1");
	FAIL_IF_NOT_EQUAL_STR(onion_request_get_header(req,"Cookie"), "key1=value1; key2=value2;");

	FAIL_IF_NOT_EQUAL_STR(onion_request_get_cookie(req,"key1"), "value1");
	FAIL_IF_NOT_EQUAL_STR(onion_request_get_cookie(req,"key2"), "value2");
	FAIL_IF_EQUAL_STR(onion_request_get_cookie(req," key2"), "value2");

	onion_dict *cookies=onion_request_get_cookies_dict(req);
	FAIL_IF_EQUAL(cookies, NULL);

	FAIL_IF_NOT_EQUAL_STR(onion_dict_get(cookies,"key1"), "value1");
	FAIL_IF_NOT_EQUAL_STR(onion_dict_get(cookies,"key2"), "value2");

	onion_request_free(req);


	END_LOCAL();
}



int main(int argc, char **argv){
  START();

	setup();
	t01_create_add_free();
	t02_create_add_free_overflow();
	t03_create_add_free_full_flow();
	t04_create_add_free_GET();
	t05_create_add_free_POST();
	t06_create_add_free_POST_toobig();
	t07_multiline_headers();
  t08_sockaddr_storage();
	t09_very_long_header();
	t10_repeated_header();
	t11_cookies();

	teardown();
	END();
}
