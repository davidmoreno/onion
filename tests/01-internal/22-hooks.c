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
#include <onion.h>

/// Very basic log of request
onion_connection_status log_request(void *_, onion_request *req, onion_response *res){
  const char *method_str=onion_request_methods[onion_request_get_flags(req) & OR_METHODS];
  size_t length=-1; // Need something like onion_response_length(res) and response quering in general
  ONION_INFO("%s %d bytes", method_str, length);
  return OCS_OK; // Other code may close connection
}

/// Adds an extra header to response
onion_connection_status extra_header(void *kv_, onion_request *req, onion_response *res){
  const char **kv=(const char **)kv_;
  onion_response_add_header(res, kv[0], kv[1]);
  return OCS_OK; // Other code may close connection
}

/// Change flag dynamically after knowing the method
onion_connection_status method_ready(void, *_, onion_request *req, onion_response *res){
  if (onion_request_get_method(req)==PUT){
    // onion_request_set_flag(req, OF_PUT_INTO_FILE); // Needs this functionality
  }
  return OCS_OK; // Other code may close connection
}

void t01_hooks_api(){
  INIT_LOCAL();

  onion *o=onion_new(O_POOL);

  int log_request_id=onion_hook_add(OH_AFTER_REQUEST_HANDLER, log_request, NULL, NULL);
  int extra_headers_id=onion_hook_add(OH_AFTER_REQUEST_HANDLER, extra_header, [ "Extra-Header", "OK" ], NULL);

  int method_ready_id=onion_hook_add(OH_ON_METHOD_READY, method_ready, NULL, NULL);

  // onion_listen(o);

  onion_hook_remove(log_request_id);
  onion_hook_remove(extra_headers_id);

  // Of course at onion_free also freed all data.
  //onion_hook_remove(method_ready_id);
  onion_free(o);

  END_LOCAL();
}

int main(int argc, char **argv){
  START();

  t01_hooks_api();

  END();
}
