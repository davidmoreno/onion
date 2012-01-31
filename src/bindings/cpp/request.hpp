/*
  Onion HTTP server library
  Copyright (C) 2010-2012 David Moreno Montero

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 3.0 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not see <http://www.gnu.org/licenses/>.
  */

#ifndef __REQUEST_HPP__
#define __REQUEST_HPP__

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
      return Dict(onion_request_get_post_dict(ptr));
    }
    const Dict query() const{
      return Dict(onion_request_get_query_dict(ptr));
    }
    const Dict session() const{
      return Dict(onion_request_get_session_dict(ptr));
    }
  };
}

#endif
