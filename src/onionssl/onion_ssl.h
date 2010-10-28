/*
	Onion HTTP server library
	Copyright (C) 2010 David Moreno Montero

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Affero General Public License as
	published by the Free Software Foundation, either version 3 of the
	License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Affero General Public License for more details.

	You should have received a copy of the GNU Affero General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
	*/

#ifndef __ONION_SSL__
#define __ONION_SSL__

#ifdef __cplusplus
extern "C"{
#endif

#include <onion_types.h>

/// Flags for the SSL connection.
enum onion_ssl_flags_e{
	O_USE_DEV_RANDOM=0x0100,
};

typedef enum onion_ssl_flags_e onion_ssl_flags;

/// Types of certificate onionssl knows: key, cert and intermediate
enum onion_ssl_certificate_type_t{
	O_SSL_CERTIFICATE_KEY=1,		/// The certfile, and the key file. 
	O_SSL_CERTIFICATE_CRL=2,		/// Certificate revocation list
	O_SSL_CERTIFICATE_TRUST=3,	/// The list of trusted CAs, also known as intermediaries.
	O_SSL_CERTIFICATE_PKCS12=4,	/// The certificate is in a PKCS12. Needs the PKCS12 file and the password. Set password=NULL if none.
	
	O_SSL_DER=0x0100, 					/// The certificate is in memory, not in a file. Default is PEM.
	O_SSL_NO_DEINIT=0x0200, 		/// Should not deinit GnuTLS at free. Use only if there are more users of GnuTLS on this executable. Saves some memory on free.
};

typedef enum onion_ssl_certificate_type_t onion_ssl_certificate_type;

struct onion_ssl_t;
typedef struct onion_ssl_t onion_ssl;

/// Creates the onion structure to fill with the server data, and later do the onion_listen()
onion_ssl *onion_ssl_new(int flags);

/// Set a certificate for use in the connection
int onion_ssl_use_certificate(onion_ssl *onion, onion_ssl_certificate_type type, const char *filename, ...);

/// Performs the listening with the given mode
int onion_ssl_listen(onion_ssl *server);

/// Removes the allocated data
void onion_ssl_free(onion_ssl *onion);

/// Sets the root handler
void onion_ssl_set_root_handler(onion_ssl *server, onion_handler *handler);

/// Sets the port to listen
void onion_ssl_set_port(onion_ssl *server, int port);

#ifdef __cplusplus
}
#endif

#endif
