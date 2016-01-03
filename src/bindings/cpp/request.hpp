/*
	Onion HTTP server library
	Copyright (C) 2010-2014 David Moreno Montero and othes

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

#ifndef ONION_REQUEST_HPP
#define ONION_REQUEST_HPP

#include "dict.hpp"

#include <string>
#include <onion/request.h>
#include <onion/low.h>

namespace Onion{
	/**
	 * @short Data about a HTTP request.
	 * 
	 * It contains all the data about headers, path, query, post, files...
	 */
	class Request{
		onion_request *ptr;
	public:
		Request(onion_request *_ptr) : ptr(_ptr){}
		
		/**
		 * @short Gets some datastraight from the headers.
		 */
		std::string operator[](const std::string &h){
			return headers()[h];
		}
		
		/**
		 * @short Returns the headers dictionary.
		 */
		const Dict headers() const{
			return Dict(onion_request_get_header_dict(ptr));
		}
		/**
		 * @short Returns the post dictionary.
		 * 
		 * If no dictionary, it returns an empty one.
		 */
		const Dict post() const{
			const onion_dict *d=onion_request_get_post_dict(ptr);
			if (d)
				return Dict(d);
			else
				return Dict();
		}
		/**
		 * @short Get a key from post
		 * 
		 * If no dictionary or not at dictionary, it returns an empty string.
		 */
		const std::string post(const std::string &key, const std::string &def=std::string()) const{
			auto r=onion_request_get_post(ptr, key.c_str());
			if (!r)
				return def;
			return r;
		}
		/**
		 * @short Returns the query dictionary, AKA get dictionary.
		 * 
		 * Contains all elements encoded in the URL (?a=b&c=d...)
		 */
		const Dict query() const{
			return Dict(onion_request_get_query_dict(ptr));
		}
		/**
		 * @short Get a key from get query
		 * 
		 * If no dictionary or not at dictionary, it returns an empty string.
		 */
		const std::string query(const std::string &key, const std::string &def=std::string()) const{
			auto r=onion_request_get_query(ptr, key.c_str());
			if (!r)
				return def;
			return r;
		}
		/**
		 * @short Returns the dictionary of the session.
		 */
		Dict session() const{
			onion_dict *d=onion_request_get_session_dict(ptr);
			if (d)
				return Dict(d);
			else
				return Dict();
		}
		/**
		 * @short Get a key from session
		 * 
		 * If no dictionary or not at dictionary, it returns an empty string.
		 */
		const std::string session(const std::string &key, const std::string &def=std::string()) const{
			auto r=onion_request_get_session(ptr, key.c_str());
			if (!r)
				return def;
			return r;
		}
		/**
		 * @short Removes current session
		 */
		void removeSession(){
			onion_request_session_free(ptr);
		}
		
		/**
		 * @short Returns a dictionary with the files. 
		 * 
		 * This dictionary has the mapping from the field name to the file in the filesystem. The file name is on the post() dictionary.
		 */
		const Dict files() const{
			const onion_dict *d=onion_request_get_file_dict(ptr);
			if (d)
				return Dict(d);
			else
				return Dict();
		}
		/**
		 * @short Checks if this request has any file POST involved.
		 */
		bool hasFiles() const{
			return (onion_request_get_file_dict(ptr)) ? true : false;
		}
		/**
		 * @short Returns the current path.
		 * 
		 * Path is cutted as understood by the diferent handler layers, so this contains the 
		 * portion important for this handler on.
		 * 
		 * In practice this means that if you have several handlers, on for "myapp/", other for "users/",
		 * and finally you get this petition for "myapp/users/XXX", on this path there is only a XXX.
		 */
		std::string path() const{
			return onion_request_get_path(ptr);
		}
		
		/**
		 * @short Returns the full path for this petition, as originally asked by the client.
		 */
		std::string fullpath() const{
			return onion_request_get_fullpath(ptr);
		}

		/**
		 * @short Returns the request flags
		 */
		onion_request_flags flags() const{
			return onion_request_get_flags(ptr);
		}

		/**
		 * @short Returns the cookies
		 */
		Dict cookies() const{
			onion_dict* d = onion_request_get_cookies_dict(ptr);
			return Dict(d);
		}

		/**
		 * @short Get a cookie
		 *
		 * If no cookie or not a dictonary, it returns an empty string.
		 */
		const std::string cookies(const std::string &key, const std::string &def=std::string()) const{
			auto r=onion_request_get_cookie(ptr, key.c_str());
			if (!r)
				return def;
			return r;
		}

		/**
		 * @short Returns a string with the client description
		 */
		const std::string clientDescription() const{
			return onion_request_get_client_description(ptr);
		}

		/**
		 * @short Returns the language code for the current request.
                 * If none, returns "C".
                 *
                 * Language code is short code, no localization at this time.
		 */
		const std::string languageCode() const{
                        /* This has code smell, but according to docs from
                         * onion/request.c, the returned value must be freed.
                         * So, initialize a new string which copies the data,
                         * free the returned value, and return the new string.
                         */
			const char* r = onion_request_get_language_code(ptr);
			std::string retval(r);
			free(const_cast<char*>(r));
			return retval;
		}

		/**
		 * @short Returns the secure status of the request
		 */
		bool isSecure() const {
			return onion_request_is_secure(ptr);
		}

		/**
		 * @short Returns the C handler, to use C functions.
		 */
		onion_request *c_handler(){
			return ptr;
		}
	};
}

#endif
