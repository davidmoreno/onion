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

#ifndef ONION_RESPONSE_HPP
#define ONION_RESPONSE_HPP

#include <onion/log.h>
#include <onion/response.h>
#include <ostream>

namespace Onion{
  class Response : public std::ostream {
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
    
    int write(const char *data, int len){
      return onion_response_write(ptr, data, len);
    }
    
    void setHeader(const std::string &k, const std::string &v){
      onion_response_set_header(ptr, k.c_str(), v.c_str());
    }
    
    void setLength(size_t length){
      onion_response_set_length(ptr,length);
    }
    
    void setCode(int code){
      onion_response_set_code(ptr,code);
    }
    
    void writeHeaders(){
			onion_response_write_headers(ptr);
		}
    
    onion_response *c_handler(){
			return ptr;
		}
  };
  
}

#endif
