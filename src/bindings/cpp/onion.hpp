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

#ifndef ONION_ONION_HPP
#define ONION_ONION_HPP

#include <string>
#include <onion/onion.h>
#include "handler.hpp"

namespace Onion{
  
  class Onion{
    onion *ptr;
  public:
    typedef int Flags;
    
    Onion(Flags f=O_POLL){
      ptr=onion_new(f);
    }
    ~Onion(){
      onion_free(ptr);
    }
    
    bool listen(){
      return onion_listen(ptr);
    }
    
    void listenStop(){
      onion_listen_stop(ptr);
    }
    
    void setRootHandler(Handler *handler){
      onion_set_root_handler(ptr, handler->c_handler());
    }
    
    void setInternalErrorHandler(Handler *handler){
      onion_set_internal_error_handler(ptr, handler->c_handler());
    }
    
    void setPort(const std::string &portName){
      onion_set_port(ptr,portName.c_str());
    }
    
    void setHostname(const std::string &hostname){
      onion_set_hostname(ptr, hostname.c_str());      
    }
    
    Flags flags(){
      return onion_flags(ptr);
    }
    
    void setTimeout(int timeout){
      onion_set_timeout(ptr, timeout);
    }
    
    onion *c_handler(){
      return ptr;
    }
  };
}
      
      
#endif
