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

#ifndef ONION_HANDLER_HPP
#define ONION_HANDLER_HPP

#include <onion/handler.h>
#include <exception>
#include <string>
#include <functional>

namespace Onion{
  class Request;
  class Response;
  
  class Handler{
    onion_handler *ptr;
  public:
    Handler();
    virtual ~Handler();
    
    virtual onion_connection_status operator()(Onion::Request &, Onion::Response &) = 0;
    
    onion_handler *c_handler(){ return ptr; }
  };
  
  template<class T>
  class HandlerMethod : public Handler{
  public:
    typedef onion_connection_status (T::*fn_t)(Onion::Request&,Onion::Response&);
  private:
    T *obj;
    fn_t fn;
  public:
    HandlerMethod(T *_obj, fn_t _fn) : obj(_obj), fn(_fn) {} ;
    virtual onion_connection_status operator()(Onion::Request &req, Onion::Response &res){
      return (obj->*fn)(req, res);
    }
  };
  
  class HandlerFunction : public Handler{
  public:
    typedef std::function<onion_connection_status (Onion::Request&,Onion::Response&)> fn_t;
  private:
    fn_t fn;
  public:
    HandlerFunction(fn_t _fn) : fn(_fn){}
    virtual onion_connection_status operator()(Onion::Request &req, Onion::Response &res){
      return fn(req, res);
    }
  };
	
  class HandlerCFunction : public Handler{
  private:
    onion_handler_handler fn;
  public:
    HandlerCFunction(onion_handler_handler _fn) : fn(_fn){}
    virtual onion_connection_status operator()(Onion::Request &req, Onion::Response &res);
  };
	
	class HttpException : public std::exception{
		std::string str;
	public:
		HttpException(const std::string &str) : str(str){}
		virtual ~HttpException() throw(){}
		const char *what() const throw(){ return str.c_str(); }
		const std::string &message(){ return str; }
	};
	
	class HttpInternalError : public HttpException{
	public:
		HttpInternalError(const std::string &str) : HttpException(str){};
		virtual ~HttpInternalError() throw(){}
	};
	
	class HttpNotFound : public HttpException{
	public:
		HttpNotFound(const std::string &str) : HttpException(str){};
		virtual ~HttpNotFound() throw(){}
	};

	class HttpRedirect : public HttpException{
	public:
		HttpRedirect(const std::string &url) : HttpException(url){};
		virtual ~HttpRedirect() throw(){}
	};
	
};

#endif
