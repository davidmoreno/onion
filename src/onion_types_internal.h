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

#include "onion_types.h"

#ifdef __cplusplus
extern "C"{
#endif

/**
 * @short A 'char *' to 'char *' dictionary.
 *
 * Internally its structured as a binary tree.
 */
struct onion_dict_t{
	const char *key;
	const char *value;
	char flags;
	struct onion_dict_t *left;
	struct onion_dict_t *right;
};

/**
 * @short Basic structure that contains the webserver info.
 */
struct onion_t{
	int flags;
	int listenfd;
	onion_server *server;
	int port;
};


/**
 * @short Some configuration about the server that should arrive to all parts.
 */
struct onion_server_t{
	onion_write write; // of type onion_write
	onion_handler *root_handler;
};

/**
 * @short Basic information about a request
 */
struct onion_request_t{
	onion_server *server; /// Server original data, like write function
	onion_dict *headers;  /// Headers prepared for this response.
	int flags;            /// Flags for this response. Ored onion_request_flags_e
	char *fullpath;       /// Original path for the request
	char *path;           /// Path at this level. Its actually a pointer inside fullpath, removing the leading parts already processed by handlers
	onion_dict *query;    /// When the query (?q=query) is processed, the dict with the values @see onion_request_parse_query
	onion_write write;    /// Write function
	void *socket;         /// Write function handler
	char buffer[128];     /// Buffer for queries. This should be enough. UGLY. FIXME.
	int buffer_pos;
};


/**
 * @short The response
 */
struct onion_response_t{
	onion_request *request;
	onion_dict *headers;
	int code;
	int flags;
	unsigned int length;
	unsigned int sent_bytes;
};


#ifdef __cplusplus
}
#endif

#endif
