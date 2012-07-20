#include "../ctest.h"
#include "buffer_listen_point.h"
#include <onion/log.h>
#include <onion/onion.h>
#include <onion/dict.h>
#include <onion/types_internal.h>

onion_connection_status _13_otemplate_html_handler_page(onion_dict *context, onion_request *req, onion_response *res);

struct tests_call_otemplate{
  char ok_title;
  char ok_list;
  char ok_hello;
  char ok_title_title;
};

ssize_t test_call_otemplate_writer(onion_request *ret, const char *data, size_t length){
	struct tests_call_otemplate *test=ret->connection.user_data;
  ONION_DEBUG("Server writes %d: %s", length, data);
  if (strstr(data, "TITLE"))
    test->ok_title=1;
  if (strstr(data,"LIST"))
    test->ok_list=1;
  if (strstr(data,"{hello}"))
    test->ok_hello=1;
  if (strstr(data,"TITLE TITLE"))
    test->ok_title_title=1;
  
  return length;
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
	lp->write=test_call_otemplate_writer;
  
  struct tests_call_otemplate tests= { -1, -1, -1, -1 };
  
  onion_request *req=onion_request_new(lp);
	req->connection.user_data=&tests;
  FAIL_IF_NOT_EQUAL_INT(onion_request_write0(req, "GET /\n\n"), OCS_CLOSE_CONNECTION);
  
  FAIL_IF_NOT_EQUAL_INT(tests.ok_hello,1);
  FAIL_IF_NOT_EQUAL_INT(tests.ok_list,-1);
  FAIL_IF_NOT_EQUAL_INT(tests.ok_title,-1);
  FAIL_IF_NOT_EQUAL_INT(tests.ok_title_title,-1);

  
  onion_dict *d=onion_dict_new();
  onion_dict_add(d, "title", "TITLE",0);
  onion_dict_add(d, "hello", "SHOULD NOT APPEAR",0);
  
  memset(&tests,0,sizeof(tests));
  onion_request_clean(req);
  onion_set_root_handler(s, onion_handler_new((void*)_13_otemplate_html_handler_page, d, (void*)onion_dict_free));
  FAIL_IF_NOT_EQUAL_INT(onion_request_write0(req, "GET /\n\n"), OCS_CLOSE_CONNECTION);
  
  FAIL_IF_NOT_EQUAL_INT(tests.ok_hello,1);
  FAIL_IF_NOT_EQUAL_INT(tests.ok_list,0);
  FAIL_IF_NOT_EQUAL_INT(tests.ok_title,1);
  FAIL_IF_NOT_EQUAL_INT(tests.ok_title_title,1);

  
  onion_dict *d2=onion_dict_new();
  onion_dict_add(d2,"0","LIST 1",0);
  onion_dict_add(d2,"1","LIST 2",0);
  onion_dict_add(d2,"2","LIST 3",0);
  onion_dict_add(d,"list",d2, OD_DICT|OD_FREE_VALUE);
  
  memset(&tests,0,sizeof(tests));
  onion_request_clean(req);
  onion_set_root_handler(s, onion_handler_new((void*)_13_otemplate_html_handler_page, d, (void*)onion_dict_free));
  FAIL_IF_NOT_EQUAL_INT(onion_request_write0(req, "GET /\n\n"), OCS_CLOSE_CONNECTION);
  
  FAIL_IF_NOT_EQUAL_INT(tests.ok_hello,1);
  FAIL_IF_NOT_EQUAL_INT(tests.ok_list,1);
  FAIL_IF_NOT_EQUAL_INT(tests.ok_title,1);
  FAIL_IF_NOT_EQUAL_INT(tests.ok_title_title,1);

  
  onion_request_free(req);
  onion_free(s);
  
  END_LOCAL();
}

int main(int argc, char **argv){
  START();
  
  t01_call_otemplate();
  
  END();
}
