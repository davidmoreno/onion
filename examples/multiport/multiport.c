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
#include <onion/http.h>
#include <onion/https.h>
#include <onion/log.h>

#include <onion/handlers/exportlocal.h>
#include <onion/handlers/auth_pam.h>
#include <signal.h>

onion *o = NULL;

void free_onion(int unused) {
  ONION_INFO("Closing connections");
  onion_free(o);
  exit(0);
}

int main(int argc, char **argv) {
  o = onion_new(O_THREADED);
  signal(SIGINT, free_onion);

  onion_set_root_handler(o, onion_handler_export_local_new("."));
  onion_add_listen_point(o, "localhost", "8080", onion_http_new());
  onion_add_listen_point(o, "localhost", "8081", onion_http_new());
#ifdef HAVE_GNUTLS
  onion_add_listen_point(o, "localhost", "4443",
                         onion_https_new(O_SSL_CERTIFICATE_KEY, "cert.pem",
                                         "cert.key"));
#else
  ONION_WARNING("HTTPS support is not enabled. Recompile with gnutls");
#endif

        /**
	onion_set_port(o, "localhost", "6121", onion_protocol_spdy());
	*/
  onion_listen(o);
  onion_free(o);
  return 0;
}
