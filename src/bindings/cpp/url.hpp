/*
	Onion HTTP server library
	Copyright (C) 2010-2013 David Moreno Montero

	This library is free software; you can redistribute it and/or
	modify it under the terms of, at your choice:
	
	a. the GNU Lesser General Public License as published by the 
	 Free Software Foundation; either version 3.0 of the License, 
	 or (at your option) any later version.
	
	b. the GNU General Public License as published by the 
	 Free Software Foundation; either version 2.0 of the License, 
	 or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License and the GNU General Public License along with this 
	library; if not see <http://www.gnu.org/licenses/>.
	*/

#ifndef __URL_HPP__
#define __URL_HPP__

#include "onion.hpp"
#include "handler.hpp"
#include <onion/url.h>
#include <onion/onion.h>

namespace Onion{
  class Url{
    onion_url *ptr;
  public:
    Url(onion_url *_ptr) : ptr(_ptr){};
    Url(Onion *o) {
      ptr=onion_root_url(o->c_handler());
    }
    Url(Onion &o) {
      ptr=onion_root_url(o.c_handler());
    }

    onion_url *c_handler(){ return ptr;  }
    
    bool add(const std::string &url, Handler *h){
      return onion_url_add_handler(ptr,url.c_str(),h->c_handler());
    }
    bool add(const std::string &url, HandlerFunction::fn_t fn){
      return add(url,new HandlerFunction(fn));
    }
    template<class T>
    bool add(const std::string &url, T *o, onion_connection_status (T::*fn)(Request &,Response &)){
      return add(url,new HandlerMethod<T>(o,fn));
    }
    bool add(const std::string &url, const std::string &s, int http_code=200){
      return onion_url_add_static(ptr,url.c_str(),s.c_str(),http_code);
    }
    bool add(const std::string &url, onion_handler_handler handler){
			return add(url, new HandlerCFunction(handler));
		}
  };
}

#endif
