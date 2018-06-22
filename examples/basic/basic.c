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
#include <onion/onion.h>

#include <onion/handlers/exportlocal.h>
#include <onion/handlers/auth_pam.h>

int main(int argc, char **argv) {
  onion *o = onion_new(O_THREADED);
  onion_set_certificate(o, O_SSL_CERTIFICATE_KEY, "cert.pem", "cert.key",
                        O_SSL_NONE);
  onion_set_root_handler(o,
                         onion_handler_auth_pam("Onion Example", "login",
                                                onion_handler_export_local_new
                                                (".")));
  onion_listen(o);
  onion_free(o);
  return 0;
}
