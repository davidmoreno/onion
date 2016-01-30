/*
	Onion HTTP server library
	Copyright (C) 2010-2014 David Moreno Montero and othes

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

	You should have received a copy of both libraries, if not see 
	<http://www.gnu.org/licenses/> and 
	<http://www.apache.org/licenses/LICENSE-2.0>.
	*/
#ifndef ONION_HTTPS_HPP
#define ONION_HTTPS_HPP

#include "listen_point.hpp"
#include <onion/https.h>
#include <cstdarg>

namespace Onion {
	/**
	 * @short A HTTPS listen point for an Onion::Onion object.
	 *
	 * This is a short example of how to use this object.
	 * @code
	 * Onion::Onion o;
	 * Onion::HttpsListenPoint https {};
	 * https.setCertificate(O_SSL_CERTIFICATE_KEY, "cert.pem", "cert.key");
	 * o.addListenPoint("localhost", "443", std::move(https));
	 * o.listen();
	 * @endcode
	 */
	class HttpsListenPoint : public ListenPoint {
	public:
		HttpsListenPoint() : ListenPoint(onion_https_new()) {
		}

		/**
		 * @short Sets the SSL certificate and key.
		 * This follows the same semantics as onion_https_set_certificate().
		 */
		int setCertificate(onion_ssl_certificate_type type, const char* filename, ...) {
			va_list va;
			va_start(va, filename);
			int r = onion_https_set_certificate_argv(ptr.get(), type, filename, va);
			va_end(va);
			return r;
		}
	};
}

#endif

