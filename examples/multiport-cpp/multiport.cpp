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
#include <onion.hpp>
#include <http.hpp>
#include <https.hpp>
#include <onion/log.h>

#include <shortcuts.hpp>

int main(int argc, char*argv[]){
	Onion::Onion o { O_THREADED };

	o.setRootHandler(Onion::ExportLocal("."));

	o.addListenPoint("localhost", "8080", Onion::HttpListenPoint());
	o.addListenPoint("localhost", "8081", Onion::HttpListenPoint());

#ifdef HAVE_GNUTLS
	Onion::HttpsListenPoint https;
	https.setCertificate(O_SSL_CERTIFICATE_KEY, "cert.pem", "cert.key");
	o.addListenPoint("localhost", "4443", std::move(https));
#else
	ONION_WARNING("HTTPS support is not enabled. Recompile with gnutls");
#endif

	o.listen();
	return 0;
}
