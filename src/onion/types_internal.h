/*
	Onion HTTP server library
	Copyright (C) 2010-2012 David Moreno Montero

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
	int timeout;   ///< Timeout in milliseconds
	char *username;
	onion_poller *poller;
	onion_listen_point **listen_points; ///< List of listen_point. Everytime a new listen point adds, 
														 ///< it reallocs the full list. Its NULL terminated. 
														 ///< If NULL at listen, creates a http at 8080.
	onion_handler *root_handler;	/// Root processing handler for this server.
	onion_handler *internal_error_handler;	/// Root processing handler for this server.
	size_t max_post_size;					/// Maximum size of post data. This is the sum of posts, @see onion_request_write_post
	size_t max_file_size;					/// Maximum size of files. @see onion_request_write_post
	onion_sessions *sessions;			/// Storage for sessions.
#ifdef HAVE_PTHREADS
	pthread_t *threads;
	int nthreads;
#endif
};


struct onion_request_t{
	struct{
		onion_listen_point *listen_point;
		void *user_data;
		int fd; ///< Original fd, to use at polling.
		struct sockaddr_storage cli_addr;
		socklen_t cli_len;
		char *cli_info;
	}connection;  /// Connection to the client.
	int flags;            /// Flags for this response. Ored onion_request_flags_e

	char *fullpath;       /// Original path for the request
	char *path;           /// Path at this level. Its actually a pointer inside fullpath, removing the leading parts already processed by handlers
	onion_dict *headers;  /// Headers prepared for this response.
	onion_dict *GET;      /// When the query (?q=query) is processed, the dict with the values @see onion_request_parse_query
	onion_dict *POST;     /// Dictionary with POST values
	onion_dict *FILES;    /// Dictionary with files. They are automatically saved at /tmp/ and removed at request free. mapped string is full path.
	onion_dict *session;  /// Pointer to related session
	onion_block *data;    /// Some extra data from PUT, normally PROPFIND.
	char *session_id;     /// Session id of the request, if any.
	void *parser;         /// When recieving data, where to put it. Check at request_parser.c.
	void *parser_data;    /// Data necesary while parsing, muy be deleted when state changed. At free is simply freed.
};

struct onion_response_t{
	onion_request *request;  	/// Original request, so both are related, and get connected to the onion_t structure. Writes through the request connection.
	onion_dict *headers;			/// Headers to write when appropiate.
	int code;									/// Response code
	int flags;								/// Flags. @see onion_response_flags_e
	unsigned int length;			/// Length, if known, of the response, to create the Content-Lenght header. 
	unsigned int sent_bytes; 	/// Sent bytes at content.
	unsigned int sent_bytes_total; /// Total sent bytes, including headers.
	char buffer[ONION_RESPONSE_BUFFER_SIZE]; /// buffer of output data. This way its do not send small chunks all the time, but blocks, so better network use. Also helps to keep alive connections with less than block size bytes.
	off_t buffer_pos;						/// Position in the internal buffer. When sizeof(buffer) its flushed to the onion IO.
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

struct onion_block_t{
	char *data;
	int size;
	int maxsize;
};

/// Opaque type used at onion_url internally
struct onion_url_data_t;
typedef struct onion_url_data_t onion_url_data;


struct onion_listen_point_t{
	onion *server;
	char *hostname;
	char *port;
	int listenfd;
	
	void *user_data;
	void (*free)(onion_listen_point *user_data);
	
	/// How to start the listening phase. Normally NULL means socket listening. Must set at fd a file descriptor that will be polled.
	void (*listen)(onion_listen_point *lp);
	/// Has default implementation that do the socket accept and set of default params. On some protocols may be 
	/// reimplemented to do non socket-petition accept.
	void (*request_init)(onion_request *req);

	/// @{ @name To be used by connections, but as these methods are shared by protocol, done here.
	int (*read_ready)(onion_request *con); ///< When poller detects data is ready to be read. Might be diferent in diferent parts of the processing.
	ssize_t (*write)(onion_request *con, const char *data, size_t len);
	ssize_t (*read)(onion_request *con, char *data, size_t len);
	void (*close)(onion_request *con); ///< Closes the connection and frees user data.
	/// @}
};


#ifdef __cplusplus
}
#endif

#endif
