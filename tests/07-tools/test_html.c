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

#include <libintl.h>
#include <onion/onion.h>
#include <onion/dict.h>

int work(onion_dict * context, onion_request * req) {
  onion_response *res = onion_response_new(req);
  onion_response_write_headers(res);

  onion_response_write(res, "\012<html>\012<head>\012 <title>", 23);
  onion_response_write0(res, onion_dict_get(context, "title"));
  onion_response_write(res, "</title>\012</head>\012<body>\012\012<h1>", 29);
  onion_response_write0(res, onion_dict_get(context, "title"));
  onion_response_write(res, "</h1>\012\012", 7);
  onion_response_write0(res, gettext("This is a test."));
  onion_response_write(res, "\012<p>\012", 5);
  onion_response_write0(res, gettext("Random value"));
  onion_response_write(res, " ", 1);
  onion_response_write0(res, onion_dict_get(context, "random"));
  onion_response_write(res, ".\012\012</body>\012</html>\012", 19);

  return onion_response_free(res);
}
