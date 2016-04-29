/*
	Onion HTTP server library
	Copyright (C) 2010-2016 David Moreno Montero and others

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

#ifndef ONION_TYPES_H
#define ONION_TYPES_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C"{
#endif

/**
 * @struct onion_dict_t
 * @short A 'char *' to 'char *' dictionary.
 * @ingroup dict
 */
struct onion_dict_t;
typedef struct onion_dict_t onion_dict;
/**
 * @struct onion_handler_t
 * @short Information about a handler for onion. A tree structure of handlers is what really serves the data.
 * @ingroup handler
 */
struct onion_handler_t;
typedef struct onion_handler_t onion_handler;

/**
 * @struct onion_url_t
 * @short Url regexp pack. This is also a handler, and can be converted with onion_url_to_handle.
 * @ingroup url
 */
struct onion_url_t;
typedef struct onion_url_t onion_url;

/**
 * @struct onion_request_t
 * @short Basic information about a request
 * @ingroup request
 */
struct onion_request_t;
typedef struct onion_request_t onion_request;
/**
 * @struct onion_response_t
 * @short The response
 * @ingroup response
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
 * @ingroup onion
 *
 * This is information about onion implementation of the generic server. It contains the listening descriptors,
 * the SSL parameters if SSL is enabled...
 *
 * This is platform specific server IO. Normally POSIX, using TCP/IP.
 */
struct onion_t;
typedef struct onion_t onion;
  /* Signature of the onion client data destoyer. */
typedef void (onion_client_data_free_sig) (void*);

/**
 * @struct onion_sessions_t
 * @short Storage for all sessions known
 * @ingroup sessions
*
 * This is a simple storage for sessions.
 *
 * Sessions are thread safe to use.
 *
 * The sessions themselves are not created until some data is written to it by the program. This way we avoid
 * "session attack" where a malicious user sends many petitions asking for new sessions.
 *
 * FIXME to add some LRU so that on some moment we can remove old sessions.
 */
struct onion_sessions_t;
typedef struct onion_sessions_t onion_sessions;


/**
 * @struct onion_block_t
 * @short Data type to store some raw data
 * @ingroup block
 *
 * Normally it will be used to store strings when the size is unknown beforehand,
 * but it can contain any type of data.
 *
 * Use with care as in most situations it might not be needed and more efficient
 * alternatives may exist.
 */
struct onion_block_t;
typedef struct onion_block_t onion_block;

/**
 * @struct onion_poller_t
 * @short Manages the polling on a set of file descriptors
 * @ingroup poller
 */
struct onion_poller_t;
typedef struct onion_poller_t onion_poller;

/**
 * @struct onion_poller_slot_t
 * @short Data about a poller element: timeout, function to call shutdown function
 * @ingroup onion
 */
struct onion_poller_slot_t;
typedef struct onion_poller_slot_t onion_poller_slot;


/**
 * @short Stored common data for each listen point: address, port, protocol status data...
 * @struct onion_listen_point_t
 * @memberof onion_listen_point_t
 * @ingroup listen_point
  *
 * Stored information about the listen points; where they are listenting, and how to handle
 * a new connection. Each listen point can understand a protocol and associated data.
 *
 * A protocol is HTTP, HTTPS, SPDY... each may do the request parsing in adiferent way, and the
 * response write too.
 *
 * A listen point A can be HTTPS with one certificate, and B with another, with C using SPDY.
 *
 */
struct onion_listen_point_t;
typedef struct onion_listen_point_t onion_listen_point;


/**
 * @short Websocket data type, as returned by onion_websocket_new
 * @memberof onion_websocket_t
 * @struct onion_websocket_t
 * @ingroup websocket
 *
 * FIXME: Some websocket description on how to use.
 *
 * Ping requests (client->server) are handled internally. pong answers are not (server->client).
 *
 * When a ping request is received, callback may be called with length=0, and no data waiting to be read.
 */
struct onion_websocket_t;
typedef struct onion_websocket_t onion_websocket;



/**
 * @short List of pointers.
 * @memberof onion_ptr_list_t
 * @struct onion_ptr_list_t
 * @ingroup ptr_list
 *
 * Used at least on onion_request to as a freelist;
 */
struct onion_ptr_list_t;
typedef struct onion_ptr_list_t onion_ptr_list;

/// Flags for the mode of operation of the onion server.
/// @ingroup onion
enum onion_mode_e{
	O_ONE=1,							///< Perform just one petition
	O_ONE_LOOP=3,					///< Perform one petition at a time; lineal processing
	O_THREADED=4,					///< Threaded processing, process many petitions at a time. Needs pthread support.
	O_DETACH_LISTEN=8,		///< When calling onion_listen, it returns inmediatly and do the listening on another thread. Only if threading is available.
	O_SYSTEMD=0x010,			///< Allow to start as systemd service. It try to start as if from systemd, but if not, start normally, so its "transparent".
/**
 * @short Use polling for request read, then as other flags say
 *
 * O_POLL must be used with other method, O_ONE_LOOP or O_THREAD, and it will poll for requests until the
 * request is ready. On that moment it will or just launch the request, which should not block, or
 * a new thread for this request.
 *
 * If on O_ONE_LOOP mode the request themselves can hook to the onion_poller object, and be called
 * when ready. (TODO).
 *
 */
	O_POLL=0x020, ///< Use epoll for request read, then as other flags say.
  O_POOL=0x024, ///< Create some threads, and make them listen for ready file descriptors. It is O_POLL|O_THREADED
	O_NO_SIGPIPE=0x040, ///< Since 0.7 by default onion ignores SIGPIPE signals, as they were a normal cause for program termination in undesired conditions.
	O_NO_SIGTERM=0x080, ///< Since 0.7 by default onion connect SIGTERM/SIGINT to onion_listen_stop, sice normally thats what user needs. Double Crtl-C do an abort.
	/// @{  @name From here on, they are internal. User may check them, but not set.
	O_SSL_AVAILABLE=0x0100, ///< This is set by the library when creating the onion object, if SSL support is available.
	O_SSL_ENABLED=0x0200,   ///< This is set by the library when setting the certificates, if SSL is available.

	O_THREADS_AVAILABLE=0x0400, ///< Threads are available on this onion build
	O_THREADS_ENABLED=0x0800,   ///< Threads are enabled on this onion object. It difers from O_THREADED as this is set by the library, so it states a real status, not a desired one.

	O_DETACHED=0x01000,		///< Currently listening on another thread.
	O_LISTENING=0x02000,		///< Currently listening
	/// @}
};

typedef enum onion_mode_e onion_mode;

/**
 * @short The desired connection state of the connection.
 * @ingroup handler
 *
 * If <0 it means close connection. May mean also to show something to the client.
 */
enum onion_connection_status_e{
	OCS_NOT_PROCESSED=0,
	OCS_NEED_MORE_DATA=1,
	OCS_PROCESSED=2,
	OCS_CLOSE_CONNECTION=-2,
	OCS_KEEP_ALIVE=3,
	OCS_WEBSOCKET=4,
  OCS_REQUEST_READY=5, ///< Internal. After parsing the request, it is ready to handle.
	OCS_INTERNAL_ERROR=-500,
	OCS_NOT_IMPLEMENTED=-501,
  OCS_FORBIDDEN=-502,
  OCS_YIELD=-3, ///< Do not remove the request/response from the pollers, I will manage it in another thread (for exmaple longpoll)
};

typedef enum onion_connection_status_e onion_connection_status;


/// Flags for the SSL connection.
/// @ingroup https
enum onion_ssl_flags_e{
	O_USE_DEV_RANDOM=0x0100,
};

typedef enum onion_ssl_flags_e onion_ssl_flags;

/// Types of certificate onionssl knows: key, cert and intermediate
/// @ingroup https
enum onion_ssl_certificate_type_e{
	O_SSL_NONE=0,								///< When actually nothing to set at onion_https_new.
	O_SSL_CERTIFICATE_KEY=1,		///< The certfile, and the key file.
	O_SSL_CERTIFICATE_CRL=2,		///< Certificate revocation list
	O_SSL_CERTIFICATE_TRUST=3,	///< The list of trusted CAs, also known as intermediaries.
	O_SSL_CERTIFICATE_PKCS12=4,	///< The certificate is in a PKCS12. Needs the PKCS12 file and the password. Set password=NULL if none.

	O_SSL_DER=0x0100, 					///< The certificate is in memory, not in a file. Default is PEM.
	O_SSL_NO_DEINIT=0x0200, 		///< Should not deinit GnuTLS at free. Use only if there are more users of GnuTLS on this executable. Saves some memory on free.
};

typedef enum onion_ssl_certificate_type_e onion_ssl_certificate_type;

/**
 * @short Types of fragments that websockets support
 * @memberof onion_websocket_t
 * @ingroup websocket
 */
enum onion_websocket_opcode_e{
	OWS_TEXT=1,
	OWS_BINARY=2,
	OWS_CONNECTION_CLOSE=8,
	OWS_PING=0x0a,
	OWS_PONG=0x0b
};

typedef enum onion_websocket_opcode_e onion_websocket_opcode;


/// Signature of request handlers.
/// @ingroup handler
typedef onion_connection_status (*onion_handler_handler)(void *privdata, onion_request *req, onion_response *res);
/// Signature of free function of private data of request handlers
/// @ingroup handler
typedef void (*onion_handler_private_data_free)(void *privdata);

/**
 * @short Prototype for websocket callbacks
 * @memberof onion_websocket_t
 * @ingroup websocket
 *
 * The callbacks are the functions to be called when new data is available on websockets.
 *
 * They are not mandatory, but when used they can be changed from the
 * callback itself, using onion_websocket_set_callback. If data is
 * left for read after a callback call, next callback is called with
 * that data. If same callback stays, and no data has been consumed,
 * it will not be called until new data is available.  End of input,
 * or websocket removal is notified with a negative data_ready_length.
 *
 * The private data is that of the original request handler.
 *
 * This chaining of callbacks allows easy creation of state machines.
 *
 * @returns OCS_INTERNAL_ERROR | OCS_CLOSE_CONNECTION | OCS_NEED_MORE_DATA. Other returns result in OCS_INTERNAL_ERROR.
 */
typedef onion_connection_status (*onion_websocket_callback_t)(void *privdata, onion_websocket *ws, ssize_t data_ready_length);


#ifdef __cplusplus
}
#endif

#endif
