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

#ifndef __ONION_TYPES_INTERNAL__
#define __ONION_TYPES_INTERNAL__

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


struct onion_dict_t{
	const char *key;
	const char *value;
	char flags;
	struct onion_dict_t *left;
	struct onion_dict_t *right;
};

struct onion_t{
	int flags;
	int listenfd;
	onion_server *server;
	int port;
	int timeout; 
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
};


struct onion_server_t{
	onion_write write;					 	/// Function to call to write. The request has the io handler to write to.
	onion_handler *root_handler;	/// Root processing handler for this server.
	onion_handler *internal_error_handler;	/// Root processing handler for this server.
	size_t max_post_size;					/// Maximum size of post data. This is the sum of posts, @see onion_request_write_post
	size_t max_file_size;					/// Maximum size of files. @see onion_request_write_post
};

/**
 * @short Internal post data. Only reserved when in a post.
 */
struct onion_request_post_data_t{
	char *buffer;      /// A post buffer that will be allocated only on POST requests.
	off_t pos;         /// Position on the post buffer.
	off_t last_part_start; /// Position where last part started, no marker included, nor \r\n.
	size_t size;       /// Size of the post buffer.
	char *marker;      /// Marker on the multipart data
	size_t marker_size; /// Size of the marker.
	onion_dict *post;     /// Dictionary with POST values
	onion_dict *files;    /// Dictionary with files. They are automatically saved at /tmp/ and removed at request free. mapped string is full path.
};

typedef struct onion_request_post_data_t onion_request_post_data;

struct onion_request_t{
	onion_server *server; /// Server original data, like write function
	onion_dict *headers;  /// Headers prepared for this response.
	int flags;            /// Flags for this response. Ored onion_request_flags_e
	char *fullpath;       /// Original path for the request
	char *path;           /// Path at this level. Its actually a pointer inside fullpath, removing the leading parts already processed by handlers
	onion_request_post_data *post_data; /// Extra data only used on posts
	onion_dict *query;    /// When the query (?q=query) is processed, the dict with the values @see onion_request_parse_query
	onion_write write;    /// Write function
	void *socket;         /// Write function handler
	char *client_info;    /// A string that describes the client, normally the IP.
	char buffer[ONION_REQUEST_BUFFER_SIZE];     /// Buffer for queries. This should be enough. UGLY. FIXME.
	char parse_state;     /// State at buffer parsing (0 headers, 1 POST data, 2 finished).
	off_t buffer_pos;     /// Position on the buffer
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
};

struct onion_handler_t{
	onion_handler_handler handler;  /// callback that should return an onion_response object, or NULL if im not entitled to respnse this request.
	onion_handler_private_data_free priv_data_free;  /// When freeing some memory, how to remove the private memory.
	void *priv_data;                /// Private data as needed by the parser
	
	struct onion_handler_t *next; /// If parser returns null, i try next parser. If no next parser i go up, or return an error. @see onion_tree_parser
};


#ifdef __cplusplus
}
#endif

#endif
