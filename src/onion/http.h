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

#ifndef ONION_HTTP_H
#define ONION_HTTP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

  onion_listen_point *onion_http_new();

#ifdef __cplusplus
}
#endif
#endif
