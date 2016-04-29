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

	You should have received a copy of both licenses, if not see
	<http://www.gnu.org/licenses/> and
	<http://www.apache.org/licenses/LICENSE-2.0>.
	*/

#ifndef ONION_REQUEST_H
#define ONION_REQUEST_H

#include <sys/types.h>
#include <sys/socket.h>
#include <stdbool.h>
#include "types.h"

#ifdef __cplusplus
extern "C"{
#endif

/**
 * @short Flags about the petition, including method, error status, http version.
 * @ingroup request
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
	OR_PATCH=10,

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

/// Creates a request from a listen point. Socket info and so on must be filled by user.
onion_request *onion_request_new(onion_listen_point *con);

/// Creates a request, with socket info.
onion_request *onion_request_new_from_socket(onion_listen_point *con, int fd, struct sockaddr_storage *cli_addr, socklen_t cli_len);

/// Deletes a request and all its data
void onion_request_free(onion_request *req);

// Partially fills a request. One line each time. DEPRECATED
//int onion_request_fill(onion_request *req, const char *data);

/// Reads some data from the input (net, file...) and performs the onion_request_fill
onion_connection_status onion_request_write(onion_request *req, const char *data, size_t length);

/// Gets the current path
const char *onion_request_get_path(onion_request *req);

/// Gets the full path of the request
const char *onion_request_get_fullpath(onion_request *req);

/// Gets the current flags, as in onion_request_flags_e
onion_request_flags onion_request_get_flags(onion_request *req);

/// Moves the path pointer to later in the fullpath
void onion_request_advance_path(onion_request *req, off_t addtopos);

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

/// Gets the cookies dict
onion_dict *onion_request_get_cookies_dict(onion_request *req);

/// Gets a cookie value
const char *onion_request_get_cookie(onion_request *req, const char *cookiename);

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

/// Returns extra request data, such as POST with non-form data, or PROPFIND. Needs the Content-Length request header.
const onion_block *onion_request_get_data(onion_request *req);

/// Performs final touches to the request to its ready to be processed.
void onion_request_polish(onion_request *req);

/// Executes the handler required for this request
onion_connection_status onion_request_process(onion_request *req);

/// Get a string with a client description
const char *onion_request_get_client_description(onion_request *req);

/// Get the sockaddr_storage from the client, if any.
struct sockaddr_storage *onion_request_get_sockadd_storage(onion_request *req, socklen_t *client_len);

/// Determine if the request was sent over a secure listen point
bool onion_request_is_secure(onion_request *req);
#ifdef __cplusplus
}
#endif

#endif
