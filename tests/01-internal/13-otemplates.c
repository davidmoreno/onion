/*
	Onion HTTP server library
	Copyright (C) 2010-2012 David Moreno Montero

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

#include "../ctest.h"
#include "buffer_listen_point.h"
#include <onion/log.h>
#include <onion/onion.h>
#include <onion/dict.h>
#include <onion/types_internal.h>
#include <onion/block.h>

onion_connection_status _13_otemplate_html_handler_page(onion_dict *context, onion_request *req, onion_response *res);
onion_connection_status AGPL_txt_handler_page(onion_dict *context, onion_request *req, onion_response *res);

struct tests_call_otemplate{
  char ok_title;
  char ok_list;
  char ok_hello;
  char ok_title_title;
	char ok_encoding;
	char ok_internal_loop;
};

void check_tests(onion_block *data, struct tests_call_otemplate *test){
	memset(test, 0, sizeof(*test));
	const char *tmp=onion_block_data(data);
	test->ok_title=test->ok_list=test->ok_hello=test->ok_title_title=0;
  if (strstr(tmp, "TITLE"))
    test->ok_title=1;
  if (strstr(tmp,"LIST"))
    test->ok_list=1;
  if (strstr(tmp,"{hello}"))
    test->ok_hello=1;
  if (strstr(tmp,"TITLE TITLE"))
    test->ok_title_title=1;
  if (strstr(tmp,"&lt;&quot;Hello&gt;"))
    test->ok_encoding=1;
	if (strstr(tmp,"internal loop"))
		test->ok_internal_loop=1;
}

static int onion_request_write0(onion_request *req, const char *str){
	//ONION_DEBUG("Write %d bytes", strlen(str));
	return onion_request_write(req,str,strlen(str));
}


void t01_call_otemplate(){
  INIT_LOCAL();


  onion *s=onion_new(0);

  onion_set_root_handler(s, onion_handler_new((void*)_13_otemplate_html_handler_page, NULL, NULL));
	onion_listen_point *lp=onion_buffer_listen_point_new();
	onion_add_listen_point(s,NULL,NULL,lp);

	struct tests_call_otemplate tests;

  onion_request *req=onion_request_new(lp);
  FAIL_IF_NOT_EQUAL_INT(onion_request_write0(req, "GET /\n\n"), OCS_REQUEST_READY);
  FAIL_IF_NOT_EQUAL_INT(onion_request_process(req), OCS_CLOSE_CONNECTION);

  ONION_INFO("Got %s",onion_buffer_listen_point_get_buffer_data(req));
  check_tests(onion_buffer_listen_point_get_buffer(req), &tests);

  FAIL_IF_NOT_EQUAL_INT(tests.ok_hello,1);
  FAIL_IF_NOT_EQUAL_INT(tests.ok_list,0);
  FAIL_IF_NOT_EQUAL_INT(tests.ok_title,0);
  FAIL_IF_NOT_EQUAL_INT(tests.ok_title_title,0);
  FAIL_IF_NOT_EQUAL_INT(tests.ok_encoding,0);


  onion_dict *d=onion_dict_new();
  onion_dict_add(d, "title", "TITLE",0);
  onion_dict_add(d, "hello", "SHOULD NOT APPEAR",0);
	onion_dict_add(d, "quoted", "<\"Hello>",0);

  onion_request_clean(req);
	onion_handler_free(onion_get_root_handler(s));
  onion_set_root_handler(s, onion_handler_new((void*)_13_otemplate_html_handler_page, d, NULL));
  FAIL_IF_NOT_EQUAL_INT(onion_request_write0(req, "GET /\n\n"), OCS_REQUEST_READY);
  FAIL_IF_NOT_EQUAL_INT(onion_request_process(req), OCS_CLOSE_CONNECTION);

  ONION_INFO("Got %s",onion_buffer_listen_point_get_buffer_data(req));
  check_tests(onion_buffer_listen_point_get_buffer(req), &tests);

  FAIL_IF_NOT_EQUAL_INT(tests.ok_hello,1);
  FAIL_IF_NOT_EQUAL_INT(tests.ok_list,0);
  FAIL_IF_NOT_EQUAL_INT(tests.ok_title,1);
  FAIL_IF_NOT_EQUAL_INT(tests.ok_title_title,1);
  FAIL_IF_NOT_EQUAL_INT(tests.ok_encoding,1);


  onion_dict *d2=onion_dict_new();
  onion_dict_add(d2,"0","LIST 1",0);
  onion_dict_add(d2,"1","LIST 2",0);
  onion_dict_add(d2,"2","LIST 3",0);
  onion_dict_add(d,"list",d2, OD_DICT|OD_FREE_VALUE);

	onion_dict *f1=onion_dict_new();
	onion_dict *f2=onion_dict_new();
	onion_dict_add(f2, "0", "internal",0);
	onion_dict_add(f2, "1", "loop",0);
	onion_dict_add(f1, "loop", f2, OD_DICT|OD_FREE_VALUE);

	onion_dict_add(d, "loop", f1, OD_DICT|OD_FREE_VALUE);

  onion_request_clean(req);
	onion_handler_free(onion_get_root_handler(s));
  onion_set_root_handler(s, onion_handler_new((void*)_13_otemplate_html_handler_page, d, (void*)onion_dict_free));
    FAIL_IF_NOT_EQUAL_INT(onion_request_write0(req, "GET /\n\n"), OCS_REQUEST_READY);
  FAIL_IF_NOT_EQUAL_INT(onion_request_process(req), OCS_CLOSE_CONNECTION);
  check_tests(onion_buffer_listen_point_get_buffer(req), &tests);
  ONION_INFO("Got %s",onion_buffer_listen_point_get_buffer_data(req));

	FAIL_IF_NOT_EQUAL_INT(tests.ok_hello,1);
  FAIL_IF_NOT_EQUAL_INT(tests.ok_list,1);
  FAIL_IF_NOT_EQUAL_INT(tests.ok_title,1);
  FAIL_IF_NOT_EQUAL_INT(tests.ok_title_title,1);
  FAIL_IF_NOT_EQUAL_INT(tests.ok_internal_loop,1);


  onion_request_free(req);
  onion_free(s);

  END_LOCAL();
}

ssize_t count_bytes(onion_request *req, const char *data, size_t length){
	int *count=req->connection.user_data;
	*count+=length;
	return length;
}

void t02_long_template(){
	INIT_LOCAL();
	int count=0;

	onion *s=onion_new(0);

	onion_set_root_handler(s, onion_handler_new((void*)AGPL_txt_handler_page, NULL, NULL));
	onion_listen_point *lp=onion_buffer_listen_point_new();
	onion_add_listen_point(s,NULL,NULL,lp);
	lp->write=count_bytes;


	onion_request *req=onion_request_new(lp);
	req->connection.listen_point->close(req);
	req->connection.user_data=&count;
	req->connection.listen_point->close=NULL;


  FAIL_IF_NOT_EQUAL_INT(onion_request_write0(req, "GET /\n\n"), OCS_REQUEST_READY);
  FAIL_IF_NOT_EQUAL_INT(onion_request_process(req), OCS_CLOSE_CONNECTION);

	FAIL_IF(count<30000);

	onion_request_free(req);
	onion_free(s);

	END_LOCAL();
}


int main(int argc, char **argv){
  START();

  t01_call_otemplate();
  t02_long_template();

  END();
}
