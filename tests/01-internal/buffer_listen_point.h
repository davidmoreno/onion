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
#ifndef __BUFFER_LISTEN_POINT_H__
#define __BUFFER_LISTEN_POINT_H__

#include <onion/types.h>

onion_listen_point *onion_buffer_listen_point_new();
const char *onion_buffer_listen_point_get_buffer_data(onion_request * req);
onion_block *onion_buffer_listen_point_get_buffer(onion_request * req);
ssize_t oblp_write_append(onion_request * a, const char *b, size_t size);

#endif
