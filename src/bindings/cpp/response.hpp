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

#ifndef ONION_RESPONSE_HPP
#define ONION_RESPONSE_HPP

#include <onion/log.h>
#include <onion/response.h>
#include <ostream>

namespace Onion{
	/**
	* @short Response on which to write the response to the clients.
	* 
	* Response itself is a std::ostream, so std::stream methods may be applied:
	* 
	* \code
	*   res<<"OK";
	* \endcode
	*/
	class Response : public std::ostream {
		/**
		* @short Buffer management for Onion::Response
		*/
		class ResponseBuf : public std::streambuf{
			Response *res;
		public:
			ResponseBuf(Response *_res) : res(_res){
			}
			virtual int overflow(int  c = traits_type::eof()){
				char C=c;
				res->write(&C,1);
				return traits_type::not_eof(c);
			}
			std::streamsize xsputn(const char *data, std::streamsize s){
				return res->write(data,s);
			}
		};
		
		onion_response *ptr;
		ResponseBuf resbuf;
	public:
		Response(onion_response *_ptr) : ptr(_ptr), resbuf(this) { init( &resbuf ); }
		
		/**
		 * @short Writes some data straigth to the response.
		 */
		int write(const char *data, int len){
			return onion_response_write(ptr, data, len);
		}
		
		/**
		 * @short Sets a header on the response.
		 */
		void setHeader(const std::string &k, const std::string &v){
			onion_response_set_header(ptr, k.c_str(), v.c_str());
		}
		
		/**
		 * @short Sets the response lenght. 
		 * 
		 * If not set onion has some heuristics to try to guess it, or use chunked encoding.
		 */
		void setLength(size_t length){
			onion_response_set_length(ptr,length);
		}

      /**
       * @short Sets a new cookie into the response.
       * @ingroup response
       *
       * @param cookiename Name for the cookie
       * @param cookievalue Value for the cookis
       * @param validity_t Seconds this cookie is valid (added to current datetime). -1 to do not expire, 0 to expire inmediatly.
       * @param path Cookie valid only for this path
       * @param Domain Cookie valid only for this domain (www.example.com, or *.example.com).
       * @param flags Flags from onion_cookie_flags_t, for example OC_SECURE or OC_HTTP_ONLY
       *
       *
       * If validity is 0, cookie is set to expire right now.
       */
        void addCookie(const std::string cookiename,
                       const std::string cookievalue,
                       time_t validity_t,
                       const std::string path,
                       const std::string domain,
                       int flags) {

          const char *_path = ((path.empty())?NULL:path.c_str());
          const char *_domain = ((domain.empty())?NULL:domain.c_str());
          onion_response_add_cookie(ptr,
                                    cookiename.c_str(),
                                    cookievalue.c_str(),
                                    validity_t,
                                    _path,
                                    _domain,
                                    flags);
        }
                       
		/**
		 * @short Sets the response code, by default 200.
		 */
		void setCode(int code){
			onion_response_set_code(ptr,code);
		}
		
		/**
		 * @short Ensure to write headers at this point. 
		 * 
		 * Normally automatically done when first data is written with write (or streams), but on some
		 * occasions user may want to force write in a specific moment.
		 */
		void writeHeaders(){
			onion_response_write_headers(ptr);
		}
		
		/**
		 * @short Gets the C handelr to use onion_response C functions.
		 */
		onion_response *c_handler(){
			return ptr;
		}
	};
	
}

#endif
