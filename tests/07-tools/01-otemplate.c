/**
  Onion HTTP server library
  Copyright (C) 2010-2018 David Moreno Montero and others

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

  You should have received a copy of both licenses, if not see
  <http://www.gnu.org/licenses/> and
  <http://www.apache.org/licenses/LICENSE-2.0>.
*/

#include <stdio.h>
#include <string.h>
#include <signal.h>

#include <onion/onion.h>
#include <onion/dict.h>
#include <onion/log.h>

onion *o;

int test_html_template(onion_dict * d, onion_request * req,
                       onion_response * res);
int extended_html_template(onion_dict * d, onion_request * req,
                           onion_response * res);

void free_onion(int unused) {
  ONION_INFO("Closing connections");
  onion_free(o);
  exit(0);
}

onion_connection_status test_page(void *ignore, onion_request * req,
                                  onion_response * res) {
  onion_dict *dict = onion_dict_new();

  char tmp[16];
  snprintf(tmp, sizeof(tmp), "%d", rand());

  onion_dict_add(dict, "title", "Test page - works", 0);
  onion_dict_add(dict, "random", tmp, OD_DUP_VALUE);

  onion_dict *subd = onion_dict_new();
  onion_dict_add(subd, "0", "Hello", 0);
  onion_dict_add(subd, "0", "World!", 0);
  onion_dict_add(dict, "subd", subd, OD_DICT | OD_FREE_VALUE);

  return test_html_template(dict, req, res);
}

onion_connection_status test2_page(void *ignore, onion_request * req,
                                   onion_response * res) {
  onion_dict *dict = onion_dict_new();

  char tmp[16];
  snprintf(tmp, sizeof(tmp), "%d", rand());

  onion_dict_add(dict, "title", "Test page - works", 0);
  onion_dict_add(dict, "random", tmp, OD_DUP_VALUE);

  onion_dict *subd = onion_dict_new();
  onion_dict_add(subd, "0", "Hello", 0);
  onion_dict_add(subd, "0", "World!", 0);
  onion_dict_add(dict, "empty", "", 0);
  onion_dict_add(dict, "subd", subd, OD_DICT | OD_FREE_VALUE);

  return extended_html_template(dict, req, res);
}

int main(int argc, char **argv) {
  o = onion_new(O_ONE_LOOP);

  onion_url *root = onion_root_url(o);
  onion_url_add_static(root, "",
                       "<a href=\"test1\">Test1</a><br><a href=\"test2\">Test2</a><br><a href=\"test3\">Test3</a>",
                       HTTP_OK);
  onion_url_add(root, "test1", test_page);
  onion_url_add(root, "test2", test2_page);
  onion_url_add(root, "test3", extended_html_template);

  signal(SIGINT, free_onion);

  onion_listen(o);

  return 0;
}
