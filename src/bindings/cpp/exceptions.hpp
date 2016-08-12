/*
	Onion HTTP server library
	Copyright (C) 2010-2015 David Moreno Montero and others

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

#pragma once

#include <exception>
#include <string>

#include <onion/types.h>
#include <onion/response.h>

namespace Onion{
	class Request;
	class Response;
	
	/**
	* @short Exception to throw if your handler failed.
	* 
	* If your handler fails and you need to signal inmediate failure, you can throw this exception.
	* 
	* This is a facility to avoid returning the onion_connection_status flags in case of error.
	*/
	class HttpException : public std::exception{
		std::string str;
		onion_response_codes code;
	public:
		HttpException(const std::string &str, onion_response_codes code=HTTP_INTERNAL_ERROR) : str(str), code(code){}
		virtual ~HttpException() throw(){}
		const char *what() const throw(){ return str.c_str(); }
		const std::string &message(){ return str; }
		/// How to handle this exception type. May be a redirect, or just plain show this message.
		virtual onion_connection_status handle(Onion::Request &req, Onion::Response &res);
	};
	
	/**
	* @short Exception to throw in case of internal errors.
	*/
	class HttpInternalError : public HttpException{
	public:
		HttpInternalError(const std::string &str) : HttpException(str){};
		virtual ~HttpInternalError() throw(){}
	};
	
	/**
	* @short Exception to throw when resource is not found, and can not be found.
	*/
	class HttpNotFound : public HttpException{
	public:
		HttpNotFound(const std::string &str) : HttpException(str){};
		virtual ~HttpNotFound() throw(){}
	};

	/**
	* @short Exception to throw to signal inmediatly that there should be a redirection.
	*/
	class HttpRedirect : public HttpException{
	public:
		HttpRedirect(const std::string &url) : HttpException(url){};
		virtual ~HttpRedirect() throw(){}
		virtual onion_connection_status handle(Request& req, Response& res);
	};
};
