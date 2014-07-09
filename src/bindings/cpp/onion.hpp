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

#ifndef ONION_ONION_HPP
#define ONION_ONION_HPP

#include <string>
#include <onion/onion.h>
#include "handler.hpp"

/**
	* This are the C++ Bindings to the C onion HTTP library.
	* 
	* Long term goal is to be able to do everything using just C++, and use as much as possible the idioms that make using C++ more
	* pleasurable than C, on the scenarios that this is possible. Anyway its a low priority project for onion, so the bindings are
	* created as needed. If you need some specific area of the onion C api converted to C++, please issue a ticket at 
	* https://github.com/davidmoreno/onion.
	* 
	* check for example https://github.com/davidmoreno/garlic for an example of a C++ onion project.
	* 
	* A simple example:
	* \code
	*   #include <onion/onion.hpp>
	*   #include <onion/response.hpp>
	*   #include <onion/url.hpp>
  *
	*   int main(int argc, char **argv){
	*     Onion::Onion o;
	* 
	*     Onion::Url root_url(o);
	* 
	*     root_url.add("",[](Onion::Request &req, Onion::Response &res){
	*       res<<"This is onion speaking";
	*       return OCS_PROCESSED;
	*     });
	* 
	*     o.listen();
	*   }
	* \endcode
*/
namespace Onion{
	
	/**
	* @short Onion server. Creates an HTTP/HTTPS server.
	* 
	* FIXME: Not all methods are implemented, just those that are needed, as they are needed. Also the API/ABI is not closed yet, so expect changes.
	* 
	* Example:
	* 
	* \code
	*   int main(int argc, char **argv){
	*     Onion::Onion o;
	* 
	*     Onion::Url root_url(o);
	* 
	*     root_url.add("",std::string("Hello world!"));
	* 
	*     o.listen();
	*   }
	* \endcode
	*/
	class Onion{
		onion *ptr;
	public:
		typedef int Flags;
		
		/**
		* @short Creates the onion server. 
		* 
		* Default flags are O_POLL, but O_POOL might be a more sensible value.
		*/
		Onion(Flags f=O_POLL){
			ptr=onion_new(f);
		}
		~Onion(){
			onion_free(ptr);
		}
		
		/**
		* @short Starts the listening for connections.
		* 
		* This is a blocking call (check O_THREADED), and can be cancelled calling listenStop.
		*/
		bool listen(){
			return onion_listen(ptr);
		}
		
		/**
		* @short Stops the listening for connections.
		*/
		void listenStop(){
			onion_listen_stop(ptr);
		}
		
		/**
		* @short Sets the root handler.
		* 
		* The root handler is the entry point for all petitions, so it should redirect to other handlers as needed.
		* Normally user dont set this handler manually, but use the Onion::Url constructor to set a Url handler as
		* the root one.
		*/
		void setRootHandler(Handler *handler){
			onion_set_root_handler(ptr, handler->c_handler());
		}
		
		/**
		* @short Sets the internal error handler, to be called in case of internal errors, not found and so on.
		*/
		void setInternalErrorHandler(Handler *handler){
			onion_set_internal_error_handler(ptr, handler->c_handler());
		}
		
		/**
		* @short Sets the port to listen to. 
		* 
		* It is a string so you can use services as listed in /etc/services.
		*/
		void setPort(const std::string &portName){
			onion_set_port(ptr,portName.c_str());
		}
		/**
		* @short Set the listening port using an integer. 
		* 
		* Its a convenience function that converts the integer to a string.
		*/
		void setPort(int port){
			std::string port_str=std::to_string(port);
			onion_set_port(ptr,port_str.c_str());
		}
		
		/**
		* @short Sets the host name to listen to. 
		* 
		* It might be a IPv4 addres, a IPv6 or a server name as in /etc/hosts.
		*/
		void setHostname(const std::string &hostname){
			onion_set_hostname(ptr, hostname.c_str());      
		}
		
		/**
		 * @short Sets the HTTPS certificates
		 */
		bool setCertificate(onion_ssl_certificate_type type, const char *filename, ...){
			va_list va;
			va_start(va, filename);
			int r=onion_set_certificate_va(ptr, type, filename, va);
			va_end(va);
			return r;
		}
		
		/**
		* @short Returns the current flags.
		*/
		Flags flags(){
			return onion_flags(ptr);
		}
		
		/**
		* @short Sets the connection timeout.
		*/
		void setTimeout(int timeout){
			onion_set_timeout(ptr, timeout);
		}
		
		/**
		* @short Returns the C handler, to use the C api.
		*/
		onion *c_handler(){
			return ptr;
		}
	};
}
			
      
#endif
