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

#ifndef ONION_HANDLER_HPP
#define ONION_HANDLER_HPP

#include <onion/handler.h>
#include <onion/response.h>
#include <string>
#include <functional>
#include <memory>

namespace Onion{
	class Request;
	class Response;

	/**
	* @short Base class to create new handlers. 
	* 
	* All onion C++ handlers must inherit from this class and provide a () operator. It will be 
	* called when the handler should be called.
	* 
	* A handler is not copyable, only movable.
	*/
	class HandlerBase{
	public:
		HandlerBase(){};
		virtual ~HandlerBase(){};
		virtual onion_connection_status operator()(Onion::Request &, Onion::Response &) = 0;
	
		HandlerBase(HandlerBase &) = delete;
		HandlerBase &operator=(HandlerBase &o) = delete;
	};

	class Handler{
		std::unique_ptr<HandlerBase> ptr;
	public:
		Handler(HandlerBase *o){
			ptr=std::unique_ptr<HandlerBase>(o);
		}
		template<typename T>
		Handler(std::unique_ptr<T> &&o){
			ptr=std::move(o);
		}
		HandlerBase *release(){ return ptr.release(); }

		template<typename T, typename ...Args>
		static std::unique_ptr<T> make(Args&&... args){
			return std::unique_ptr<T>(new T(args...));
		}
	};
	
	/// Converts a C++ handler to C
	onion_handler *onion_handler_cpp_to_c(Handler handler);
	
	/// Converts a C handler to C++
	Handler onion_handler_c_to_cpp(onion_handler *);
	
	/**
	* @short Creates a handler that calls a method in an object.
	* 
	* For example:
	* 
	*   class MyClass{
	*   public:
	*     onion_connection_status index(Onion::Request &req, Onion::Response &res);
	*   };
	* 
	*   ...
	* 
	*   MyClass class;
	*   new Onion::HandlerMethod(&class, MyClass::index);
	*/
	template<class T>
	class HandlerMethod : public HandlerBase{
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
	
	/**
	* @short Creates a Handler that just calls a function.
	* 
	* The function signature is onion_connection_status (Onion::Request&,Onion::Response&);
	*/
	class HandlerFunction : public HandlerBase{
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
	
	/**
	* @short Creates a Handler that calls a C function.
	* 
	* The C function needs the signature onion_handler_handler, which is
	* onion_connection_status handler(void *data, onion_request *req, onion_response *res);
	*/
	class HandlerCFunction : public HandlerBase{
	private:
		onion_handler_handler fn;
	public:
		HandlerCFunction(onion_handler_handler _fn) : fn(_fn){}
		virtual onion_connection_status operator()(Onion::Request &req, Onion::Response &res);
	};
	
	/**
	 * @short Calls a C handler.
	 */
	class HandlerCBridge : public HandlerBase{
		onion_handler *ptr;
	public:
		HandlerCBridge(onion_handler *handler) : ptr(handler){};
		virtual ~HandlerCBridge(){
			onion_handler_free(ptr);
		};
		virtual onion_connection_status operator()(Request &req, Response &res);
	};
};

#endif
