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

#ifndef __ONION_TYPES__
#define __ONION_TYPES__

struct onion_dict_t;
typedef struct onion_dict_t onion_dict;
struct onion_handler_t;
typedef struct onion_handler_t onion_handler;
struct onion_request_t;
typedef struct onion_request_t onion_request;
struct onion_response_t;
typedef struct onion_response_t onion_response;
struct onion_server_t;
typedef struct onion_server_t onion_server;
struct onion_t;
typedef struct onion_t onion;


typedef int (*onion_handler_handler)(onion_handler *handler, onion_request *);
typedef void (*onion_handler_private_data_free)(void *privdata);

/**
 * @short Prototype for the writing on the socket function.
 *
 * It can safely be just fwrite, with handler the FILE*, or write with the handler the fd.
 * But its usefull its like this so we can use another more complex to support, for example,
 * SSL.
 */
typedef int (*onion_write)(void *handler, const char *data, unsigned int length);

/// Flags for the mode of operation of the onion server.
enum onion_mode_e{
	O_ONE=1,							/// Perform just one petition
	O_ONE_LOOP=3,					/// Perform one petition at a time; lineal processing
	O_THREADED=4,					/// Threaded processing, process many petitions at a time. Needs pthread support.
	
	O_SSL_AVAILABLE=0x10, /// This is set by the library when creating the onion object, if SSL support is available.
	O_SSL_ENABLED=0x20,   /// This is set by the library when setting the certificates, if SSL is available.
};

typedef enum onion_mode_e onion_mode;

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


#endif

