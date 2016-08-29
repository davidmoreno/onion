/*
	Onion HTTP server library
	Copyright (C) 2016 David Moreno Montero

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
#include <onion/onion.h>
#include <onion/response.h>
#include <onion/request.h>
#include <onion/log.h>
#include "buffer_listen_point.h"

int ncalls=0;
/// Very basic log of request
onion_connection_status log_request(void *_, onion_request *req, onion_response *res){
  const char *method_str=onion_request_methods[onion_request_get_flags(req) & OR_METHODS];
  size_t length=onion_response_get_length(res);
  ONION_INFO("%s %d bytes", method_str, length);
  ncalls++;
  return OCS_CONTINUE; // Other code may close connection
}
/// Very basic log of request
onion_connection_status log_request_before(void *_, onion_request *req, onion_response *res){
  const char *method_str=onion_request_methods[onion_request_get_flags(req) & OR_METHODS];
  ONION_INFO("Before parse %s %s", method_str, onion_request_get_fullpath(req));
  ncalls++;
  return OCS_CONTINUE; // Other code may close connection
}

/// Adds an extra header to response
onion_connection_status extra_header(void *kv_, onion_request *req, onion_response *res){
  const char **kv=(const char **)kv_;
  onion_response_set_header(res, kv[0], kv[1]);
  return OCS_CONTINUE; // Other code may close connection
}

/// Change flag dynamically after knowing the method
onion_connection_status method_ready(void *_, onion_request *req, onion_response *res){
  if ((onion_request_get_flags(req)&OR_METHODS)==OR_PUT){
    // onion_request_set_flag(req, OF_PUT_INTO_FILE); // Needs this functionality
  }
  return OCS_CONTINUE; // Other code may close connection
}

void t01_hooks_api(){
  INIT_LOCAL();

  onion *o=onion_new(O_POOL);

  int log_request_id=onion_hook_add(o, OH_AFTER_REQUEST_HANDLER, log_request, NULL, NULL);
  const char *kv[2]={ "Extra-Header", "OK" };
  int extra_headers_id=onion_hook_add(o, OH_AFTER_REQUEST_HANDLER, extra_header, kv, NULL);

  //int _method_ready_id=
  onion_hook_add(o, OH_ON_METHOD_READY, method_ready, NULL, NULL);

  // onion_listen(o);

  onion_hook_remove(o, log_request_id);
  onion_hook_remove(o, extra_headers_id);

  // Of course at onion_free also freed all data.
  //onion_hook_remove(method_ready_id);
  onion_free(o);

  END_LOCAL();
}

void t02_hooks_calls(){
  onion *server=onion_new(0);

  ncalls=0;
  onion_hook_add(server, OH_ON_METHOD_READY, log_request_before, NULL, NULL);
  onion_hook_add(server, OH_ON_HEADERS_READY, log_request_before, NULL, NULL);
  onion_hook_add(server, OH_BEFORE_REQUEST_HANDLER, log_request_before, NULL, NULL);
  onion_hook_add(server, OH_AFTER_REQUEST_HANDLER, log_request, NULL, NULL);
  onion_hook_add(server, OH_AFTER_REQUEST_HANDLER, log_request, NULL, NULL);
  onion_hook_add(server, OH_AFTER_REQUEST_HANDLER, log_request, NULL, NULL);

  onion_url *urls=onion_root_url(server);
  onion_url_add_static(urls, "/", "OK", 200);

  onion_listen_point *lp=onion_buffer_listen_point_new();
  onion_add_listen_point(server,NULL,NULL,lp);

  onion_request *req=onion_request_new(lp);
  onion_connection_status rs=onion_request_write(req, "GET /\n\n", 8);
  FAIL_IF_NOT_EQUAL_INT(rs, OCS_REQUEST_READY);
  onion_request_process(req);

  const char *buffer=onion_buffer_listen_point_get_buffer_data(req);

  FAIL_IF_EQUAL_STR(buffer,"");
  FAIL_IF_NOT_STRSTR(buffer,"404");

  onion_request_free(req);
  onion_free(server);

  FAIL_IF_NOT_EQUAL_INT(ncalls, 6);
}

int main(int argc, char **argv){
  START();

  t01_hooks_api();
  t02_hooks_calls();

  END();
}
