/*
	Onion HTTP server library
	Copyright (C) 2012 David Moreno Montero

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 3.0 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not see <http://www.gnu.org/licenses/>.
	*/

#ifndef __ONION_HTTPS_H__
#define __ONION_HTTPS_H__

#include "types.h"

/// Flags for the SSL connection.
enum onion_ssl_flags_e{
	O_USE_DEV_RANDOM=0x0100,
};

typedef enum onion_ssl_flags_e onion_ssl_flags;

/// Types of certificate onionssl knows: key, cert and intermediate
enum onion_ssl_certificate_type_e{
	O_SSL_CERTIFICATE_KEY=1,		///< The certfile, and the key file. 
	O_SSL_CERTIFICATE_CRL=2,		///< Certificate revocation list
	O_SSL_CERTIFICATE_TRUST=3,	///< The list of trusted CAs, also known as intermediaries.
	O_SSL_CERTIFICATE_PKCS12=4,	///< The certificate is in a PKCS12. Needs the PKCS12 file and the password. Set password=NULL if none.
	
	O_SSL_DER=0x0100, 					///< The certificate is in memory, not in a file. Default is PEM.
	O_SSL_NO_DEINIT=0x0200, 		///< Should not deinit GnuTLS at free. Use only if there are more users of GnuTLS on this executable. Saves some memory on free.
};

typedef enum onion_ssl_certificate_type_e onion_ssl_certificate_type;

onion_listen_point *onion_https_new(onion_ssl_certificate_type type, const char *filename, ...);

#endif
