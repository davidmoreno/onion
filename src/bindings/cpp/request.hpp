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
  
  class Request{
    onion_request *ptr;
  public:
    Request(onion_request *_ptr) : ptr(_ptr){}
    
    std::string operator[](const std::string &h){
      return headers()[h];
    }
    
    const Dict headers() const{
      return Dict(onion_request_get_header_dict(ptr));
    }
    const Dict post() const{
			const onion_dict *d=onion_request_get_post_dict(ptr);
			if (d)
				return Dict(d);
			else
				return Dict();
    }
    const Dict query() const{
      return Dict(onion_request_get_query_dict(ptr));
    }
    Dict session() const{
			onion_dict *d=onion_request_get_session_dict(ptr);
			if (d)
				return Dict(d);
			else
				return Dict();
    }
    const Dict files() const{
      const onion_dict *d=onion_request_get_file_dict(ptr);
      if (d)
        return Dict(d);
      else
        return Dict();
    }
    bool hasFiles() const{
      return (onion_request_get_file_dict(ptr)) ? true : false;
    }
    std::string path() const{
			return onion_request_get_path(ptr);
		}
		onion_request *c_handler(){
			return ptr;
		}
  };
}

#endif
