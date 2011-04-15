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

/**
 * @struct onion_dict_t
 * @short A 'char *' to 'char *' dictionary.
 */
struct onion_dict_t;
typedef struct onion_dict_t onion_dict;
/**
 * @struct onion_handler_t
 * @short Information about a handler for onion. A tree structure of handlers is what really serves the data.
 */
struct onion_handler_t;
typedef struct onion_handler_t onion_handler;
/**
 * @struct onion_request_t
 * @short Basic information about a request
 */
struct onion_request_t;
typedef struct onion_request_t onion_request;
/**
 * @struct onion_response_t
 * @short The response
 */
struct onion_response_t;
typedef struct onion_response_t onion_response;
/**
 * @struct onion_server_t
 * @short Onion server that do not depend on specific IO structure.
 * 
 * This is separated as you can build your own servers using this structure instead of onion_t. For example
 * using onion_server_t you can do a inet daemon that listens HTTP data.
 */
struct onion_server_t;
typedef struct onion_server_t onion_server;
/**
 * @struct onion_t
 * @short Webserver info.
 * 
 * This is information about onion implementation of the generic server. It contains the listening descriptors,
 * the SSL parameters if SSL is enabled... 
 * 
 * This is platform specific server IO. Normally POSIX, using TCP/IP.
 */
struct onion_t;
typedef struct onion_t onion;
/**
 * @struct onion_sessions_t
 * @short Storage for all sessions known
 * 
 * This is a simple storage for sessions.
 * 
 * There is no thread safety mesaures on the usage of sessions. FIXME.
 * 
 * The sessions themselves are not created until some data is written to it by the program. This way we avoid
 * "session attack" where a malicious user sends many petitions asking for new sessions.
 * 
 * FIXME to add some LRU so that on some moment we can remove old sessions.
 */
struct onion_sessions_t;
typedef struct onion_sessions_t onion_sessions;

/// Signature of request handlers.
typedef int (*onion_handler_handler)(onion_handler *handler, onion_request *);
/// Signature of free function of private data of request handlers
typedef void (*onion_handler_private_data_free)(void *privdata);

/**
 * @short Prototype for the writing on the socket function.
 *
 * It can safely be just fwrite, with handler the FILE*, or write with the handler the fd.
 * But its useful its like this so we can use another more complex to support, for example,
 * SSL.
 */
typedef int (*onion_write)(void *handler, const char *data, unsigned int length);

/// Flags for the mode of operation of the onion server.
enum onion_mode_e{
	O_ONE=1,							///< Perform just one petition
	O_ONE_LOOP=3,					///< Perform one petition at a time; lineal processing
	O_THREADED=4,					///< Threaded processing, process many petitions at a time. Needs pthread support.
	O_DETACH_LISTEN=8,		///< When calling onion_listen, it returns inmediatly and do the listening on another thread. Only if threading is available.
	
	O_SSL_AVAILABLE=0x10, ///< This is set by the library when creating the onion object, if SSL support is available.
	O_SSL_ENABLED=0x20,   ///< This is set by the library when setting the certificates, if SSL is available.

	O_THREADS_AVALIABLE=0x40, ///< Threads are available on this onion build
	O_THREADS_ENABLED=0x80,   ///< Threads are enabled on this onion object. It difers from O_THREADED as this is set by the library, so it states a real status, not a desired one.
	
	O_DETACHED=0x0100,		///< Currently listening on another thread.
};

typedef enum onion_mode_e onion_mode;

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

/**
 * @short The desired connection state of the connection.
 * 
 * If <0 it means close connection, but may mean also to show something to the client.
 */
enum onion_connection_status_e{
	OCS_NOT_PROCESSED=0,
	OCS_NEED_MORE_DATA=1,
	OCS_CLOSE_CONNECTION=-2,
	OCS_KEEP_ALIVE=3,
	OCS_INTERNAL_ERROR=-500,
	OCS_NOT_IMPLEMENTED=-501,
};

typedef enum onion_connection_status_e onion_connection_status;


#endif

