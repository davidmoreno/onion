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

#include <onion/mime.h>
#include "../ctest.h"

void t01_test_mime() {
  INIT_LOCAL();

  FAIL_IF_NOT_EQUAL_STR("text/plain", onion_mime_get("txt"));
  FAIL_IF_NOT_EQUAL_STR("text/plain", onion_mime_get("fdsfds"));
  FAIL_IF_NOT_EQUAL_STR("text/html", onion_mime_get("html"));
  FAIL_IF_NOT_EQUAL_STR("image/png", onion_mime_get("file.png"));
  FAIL_IF_NOT_EQUAL_STR("application/javascript", onion_mime_get("js"));

  onion_mime_set(NULL);
  END_LOCAL();
}

int main(int argc, char **argv) {
  START();

  t01_test_mime();

  END();
}
