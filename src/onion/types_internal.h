/*
	Onion HTTP server library
	Copyright (C) 2010 David Moreno Montero

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

#ifndef __ONION_TYPES_INTERNAL__
#define __ONION_TYPES_INTERNAL__

#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>

#include "types.h"

#ifdef HAVE_GNUTLS
#include <gnutls/gnutls.h>
#endif
#ifdef HAVE_PTHREADS
#include <pthread.h>
#include <semaphore.h>
#endif

#ifdef __cplusplus
extern "C"{
#endif

#define ONION_REQUEST_BUFFER_SIZE 256
#define ONION_RESPONSE_BUFFER_SIZE 1500


struct onion_dict_node_t;

struct onion_dict_t{
	struct onion_dict_node_t *root;
#ifdef HAVE_PTHREADS
	pthread_rwlock_t lock;
	pthread_mutex_t refmutex;
#endif
	int refcount;
  int (*cmp)(const char *a, const char *b);
};


struct onion_t{
	int flags;
	int listenfd;
	onion_server *server;
	char *port;
	char *hostname;
	int timeout;   ///< Timeout in milliseconds
	char *username;
#ifdef HAVE_GNUTLS
	gnutls_certificate_credentials_t x509_cred;
	gnutls_dh_params_t dh_params;
	gnutls_priority_t priority_cache;
#endif
#ifdef HAVE_PTHREADS
	int max_threads;
	sem_t thread_count;
	pthread_mutex_t mutex;		/// Mutexes data on this struct.
#endif
	onion_poller *poller;
};


struct onion_server_t{
	onion_write write;					 	/// Function to call to write. The request has the io handler to write to.
	onion_read read;					 		/// Function to call to read. The request has the io handler to write to.
	onion_close close;					 		/// Function to call to close the socket.
	onion_handler *root_handler;	/// Root processing handler for this server.
	onion_handler *internal_error_handler;	/// Root processing handler for this server.
	size_t max_post_size;					/// Maximum size of post data. This is the sum of posts, @see onion_request_write_post
	size_t max_file_size;					/// Maximum size of files. @see onion_request_write_post
	onion_sessions *sessions;			/// Storage for sessions.
	struct onion_t *onion;				/// Access to parent onion.
};

struct onion_request_t{
	onion_server *server; /// Server original data, like write function
	onion_dict *headers;  /// Headers prepared for this response.
	void *socket;         /// Write function handler
	int flags;            /// Flags for this response. Ored onion_request_flags_e

	void *parser;         /// When recieving data, where to put it. Check at request_parser.c.
	void *parser_data;    /// Data necesary while parsing, muy be deleted when state changed. At free is simply freed.

	char *fullpath;       /// Original path for the request
	char *path;           /// Path at this level. Its actually a pointer inside fullpath, removing the leading parts already processed by handlers
	onion_dict *GET;      /// When the query (?q=query) is processed, the dict with the values @see onion_request_parse_query
	onion_dict *POST;     /// Dictionary with POST values
	onion_dict *FILES;    /// Dictionary with files. They are automatically saved at /tmp/ and removed at request free. mapped string is full path.
	onion_dict *session;  /// Pointer to related session
	onion_block *data;    /// Some extra data, normally PROPFIND.
	char *session_id;     /// Session id of the request, if any.
	char *client_info;    /// A string that describes the client, normally the IP.
	
	int fd; 							/// Helper that stores the original fd
	
	struct sockaddr_storage client_addr; /// Info as stored by TCP/IP, so that handlers can get as much data as needed from peer
	socklen_t client_len;	               /// Size of the sockaddr_storage as needed.
	
	onion_websocket *websocket; /// Websocket handler. 
};

struct onion_response_t{
	onion_request *request;  	/// Original request, so both are related, and get connected to the onion_server_t structure.
	onion_dict *headers;			/// Headers to write when appropiate.
	int code;									/// Response code
	int flags;								/// Flags. @see onion_response_flags_e
	unsigned int length;			/// Length, if known of the response, to create the Content-Lenght header. 
	unsigned int sent_bytes; 	/// Sent bytes at content.
	unsigned int sent_bytes_total; /// Total sent bytes, including headers.
	char buffer[ONION_RESPONSE_BUFFER_SIZE]; /// buffer of output data. This way its do not send small chunks all the time, but blocks, so better network use. Also helps to keep alive connections with less than block size bytes.
	off_t buffer_pos;						/// Position in the internal buffer. When sizeof(buffer) its flushed to the onion_server IO.
	onion_write write;    /// Write function
	void *socket;         /// Write function handler
};

struct onion_handler_t{
	onion_handler_handler handler;  /// callback that should return an onion_connection_status, and maybe process the request.
	onion_handler_private_data_free priv_data_free;  /// When freeing some memory, how to remove the private memory.
	void *priv_data;                /// Private data as needed by the handler
	
	struct onion_handler_t *next; /// If parser returns null, i try next handler. If no next handler i go up, or return an error. @see onion_handler_handle
};

// onion_url is really a handler. It has its own "fake" type to ensure user dont call the url_* functions
// by mistacke with a non url object.
// struct onion_url_t;


struct onion_sessions_t{
	onion_dict *sessions; 		/// Where all sessions are stored. Each element is another onion_dict.
};

typedef struct onion_block_t{
	char *data;
	int size;
	int maxsize;
}block;

/// Opaque type used at onion_url internally
struct onion_url_data_t;
typedef struct onion_url_data_t onion_url_data;


struct onion_websocket_t{
	onion_request *req; /// Associated request
	onion_websocket_callback_t callback; /// Callback to call, if any, when new data is available.
	
	void *user_data;
	void (*free_user_data)(void *);
	
	int64_t data_left;
	char mask[4];
	int8_t mask_pos;
	int8_t flags; /// Defined at websocket.c
	onion_websocket_opcode opcode:4;
};

#ifdef __cplusplus
}
#endif

#endif
