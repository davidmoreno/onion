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

#ifndef __ONION_REQUEST__
#define __ONION_REQUEST__

#include <sys/types.h>
#include <sys/socket.h>

#include "types.h"

#ifdef __cplusplus
extern "C"{
#endif

/**
 * @short Flags about the petition, including method, error status, http version.
 */
enum onion_request_flags_e{
  /// First 4 bytes are the method. Then as flags.
  OR_METHODS=0x0F,
	OR_GET=0,
	OR_POST=1,
	OR_HEAD=2,
	OR_OPTIONS=3,
	OR_PROPFIND=4,
	OR_PUT=5,
	OR_DELETE=6,
	OR_MOVE=7,
	OR_MKCOL=8,
	OR_PROPPATCH=9,
	
	/// Some flags at 0x0F0
	OR_HTTP11=0x10,
	OR_POST_MULTIPART=0x20,
	OR_POST_URLENCODED=0x40,
	
	/// Server flags are at 0x0F00.
	OR_NO_KEEP_ALIVE=0x0100,
  OR_HEADER_SENT_=0x0200,  ///< Dup name from onion_response_flags, same meaning.
	
	/// Errors at 0x0F000.
	OR_INTERNAL_ERROR=0x01000,
	OR_NOT_IMPLEMENTED=0x02000,
	OR_NOT_FOUND=0x03000,
  OR_FORBIDDEN=0x04000,
};

typedef enum onion_request_flags_e onion_request_flags;

/// List of known methods. NULL empty space, position is the method as listed at the flags. @see onion_request_flags
extern const char *onion_request_methods[16];

/// Creates a request with client info
onion_request *onion_request_new(onion_server *server, void *socket, const char *client_info);

/// Creates a request, with socket info.
onion_request *onion_request_new_from_socket(onion_server *server, void *socket, struct sockaddr_storage *cli_addr, socklen_t cli_len);

/// Deletes a request and all its data
void onion_request_free(onion_request *req);

/// Partially fills a request. One line each time.
int onion_request_fill(onion_request *req, const char *data);

/// Reads some data from the input (net, file...) and performs the onion_request_fill
onion_connection_status onion_request_write(onion_request *req, const char *data, size_t length);

/// Gets the current path
const char *onion_request_get_path(onion_request *req);

/// Gets the full path of the request
const char *onion_request_get_fullpath(onion_request *req);

/// Gets the current flags, as in onion_request_flags_e
onion_request_flags onion_request_get_flags(onion_request *req);

/// Moves the path pointer to later in the fullpath
void onion_request_advance_path(onion_request *req, int addtopos);

/// @{ @name Get header, query, post, file data and session

/// Gets a header data
const char *onion_request_get_header(onion_request *req, const char *header);

/// Gets query data
const char *onion_request_get_query(onion_request *req, const char *query);

/// Gets query data, but returns a default value if key not found.
const char *onion_request_get_queryd(onion_request *req, const char *key, const char *def);

/// Gets post data
const char *onion_request_get_post(onion_request *req, const char *query);

/// Gets file data
const char *onion_request_get_file(onion_request *req, const char *query);

/// Gets session data
const char *onion_request_get_session(onion_request *req, const char *query);

/// Gets the header header data dict
const onion_dict *onion_request_get_header_dict(onion_request *req);

/// Gets request query dict
const onion_dict *onion_request_get_query_dict(onion_request *req);

/// Gets post data dict
const onion_dict *onion_request_get_post_dict(onion_request *req);

/// Gets post data dict
const onion_dict *onion_request_get_file_dict(onion_request *req);

/// Gets session data dict
onion_dict *onion_request_get_session_dict(onion_request *req);

/// @}

/// Frees the session dictionary.
void onion_request_session_free(onion_request *req);

/// Cleans the request object, to reuse it
void onion_request_clean(onion_request *req);

/// Reqeust to close connection after one request is done, forces no keep alive.
void onion_request_set_no_keep_alive(onion_request *req);

/// Returns if current request wants to keep alive
int onion_request_keep_alive(onion_request *req);

/// Gets the language code for the current language. C is returned if none recognized.
const char *onion_request_get_language_code(onion_request *req);

/// Returns PROPFIND data
const onion_block *onion_request_get_data(onion_request *req);

/// Performs final touches to the request to its ready to be processed.
void onion_request_polish(onion_request *req);

/// Executes the handler required for this request
onion_connection_status onion_request_process(onion_request *req);

/// Get a string with a client description
const char *onion_request_get_client_description(onion_request *req);

#ifdef __cplusplus
}
#endif

#endif
