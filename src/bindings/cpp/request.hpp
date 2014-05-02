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
		 * @short Returns the query dictionary, AKA get dictionary.
		 * 
		 * Contains all elements encoded in the URL (?a=b&c=d...)
		 */
		const Dict query() const{
			return Dict(onion_request_get_query_dict(ptr));
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
		 * @short Returns the C handler, to use C functions.
		 */
		onion_request *c_handler(){
			return ptr;
		}
	};
}

#endif
