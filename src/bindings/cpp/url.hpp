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

#ifndef ONION_URL_HPP
#define ONION_URL_HPP

#include "onion.hpp"
#include "handler.hpp"
#include <onion/url.h>
#include <onion/log.h>
#include <onion/onion.h>

namespace Onion{
	/**
	 * @short Url management for Onion
	 * 
	 * With this class its possible to create the muxer necesary to redirect the petitions to the proper handler.
	 * 
	 * Normally user writes some regex* to map from the path to a request handler. It follows Django trail and urls can
	 * be nested, so a regex matches the start of an url, and the next handler the next part of the url. It also allows 
	 * groups as in django, but only anonymous groups.
	 * 
	 * These groups will be set at the query as fields 1, 2, 3... as found.
	 * 
	 * Example:
	 * 
	 * \code
	 *   Onion::Url url;
	 *   Onion::Url more_urls;
	 * 
	 *   url.add("", std::string("Index")); // Returns static data, just the word Index.
	 *   url.add("^more", more_urls);
	 * 
	 *   more_urls.add("", std::string("More")); // At /more/ returns "More"
	 *   more_urls.add("more", std::string("Even more")); // At /more/more/ returns "Even more".
	 * \endcode
	 * 
	 * Urls may be regex or plain strings. If plain strings it tries to match the full string. This is much faster, but
	 * limits the subURLing. If using a regex its possible to create suburls, just matching the string start: "^more". Internally
	 * this is as fast as the full string match, as it really dont use regex. So, in order of preference, use this kind of patterns:
	 * 
	 * * "^startswith"
	 * * "fullmatch"
	 * * "^regex_full_match$" 
	 * * "^regex_(group|match)$"
	 * 
	 * As it can se regex without full matching, be careful or its possible to just match substrings: "o$" matches "Hello", "Hello/World/o" and so on.
	 */
	class Url : public Handler {
		using internal_pointer = std::unique_ptr<onion_url, decltype(onion_url_free)*>;
		internal_pointer ptr;
	public:
		/**
		 * @short Creates an empty url handler.
		 */
		Url();
		/**
		 * @short Creates an url handler from the C url handler.
		 */
		Url(onion_url *_ptr);
		/**
		 * @short Creates the onion_root_handler as an Url object.
		 */
		Url(Onion *o);
		/**
		 * @short Creates the onion_root_handler as an Url object, from a onion reference.
		 */
		Url(Onion &o);
		/**
		 * @short Move constructor
		 * 
		 * Its slightly diferent from standard as it allows to keep using the original to add data; normally the state is undefined.
		 */
		Url(Url &&o);

		Url(Url &) = delete;
		Url &operator=(Url &o) = delete;

		virtual ~Url();

		/**
		 * @short Returns the C handler to use onion_url C functions.
		 */
		onion_url *c_handler();
		
		/**
		 * @short Adds an url that calls an Onion::Handler-
		 */
		Url& add(const std::string &url, Handler &&h);

		/**
		 * @short Adds an url that calls a C onion_handler.
		 */
		Url& add(const std::string &url, onion_handler *h);
		
		/**
		 * @short Adds an url that calls a C++ function.
		 * 
		 * Example:
		 * 
		 *   url.add("", [](Onion::Request &req, Onion::Response &res){ return OCS_INTERNAL_ERROR; });
		 */
		Url& add(const std::string &url, HandlerFunction::fn_t fn);

		/**
		 * @short Adds an url that calls a C++ method.
		 * 
		 * This is the most normal way to add several calls to methods on the same class.
		 * 
		 * Example:
		 * 
		 * \code
		 *   class MyClass{
		 *    public:
		 *     onion_connection_status index(Onion::Request &res, Onion::Response &res);
		 *   };
		 * 
		 *   ...
		 * 
		 *   Onion::Url url;
		 *   MyClass c;
		 *   url.add("mycall", &c, MyClass::index);
		 * \endcode
		 */
		template<class T>
		Url& add(const std::string &url, T *o, onion_connection_status (T::*fn)(Request &,Response &)){
			return add(url, (Handler&&)Handler::make<HandlerMethod<T>>(o,fn));
		}
		/**
		 * @short Adds an url with a static response.
		 */
		Url &add(const std::string &url, const std::string &s, int http_code=200);

		/**
		 * @short Adds an url that calls a C style onion handler.
		 * 
		 * With this method is possible to use the C handlers as onion_webdav.
		 */
		Url &add(const std::string &url, onion_handler_handler handler);

		/**
		 * @short Allows to call am Onion::Url to continue the processing of this request.
		 * 
		 * A use case is a have a handler that controls access by means of a session (or CSRF control), and
		 * if the conditions are OK, call the url handler to continue with the processing.
		 */
		virtual onion_connection_status operator()(Request &req, Response &res);
	};
}

#endif
