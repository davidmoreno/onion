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

#ifndef __DICT_HPP__
#define __DICT_HPP__

#include <onion/types.h>
#include <string>
#include <onion/dict.h>

namespace Onion{
  class Dict{
    onion_dict *ptr;
  public:
    class key_not_found : public std::exception{
      std::string msg;
    public:
      key_not_found(const std::string &key) : msg("Key "+key+" not found"){}
      virtual ~key_not_found() throw(){}
      const char *what() const throw(){ return msg.c_str(); }
    };
    
    Dict(onion_dict *_ptr) : ptr(_ptr){}
    Dict(const onion_dict *_ptr) : ptr((onion_dict*)_ptr){} // FIXME, loses constness
    
    std::string operator[](const std::string &k) const{
      const char *ret=onion_dict_get(ptr, k.c_str());
      if (!ret)
        throw(key_not_found(k));
      return onion_dict_get(ptr, k.c_str());
    }
    
    bool has(const std::string &k) const{
      return onion_dict_get(ptr, k.c_str())!=NULL;
    }
    
    Dict getDict(const std::string &k) const{
      return Dict(onion_dict_get_dict(ptr, k.c_str()));
    }
    
    std::string get(const std::string &k, const std::string &def="") const{
      const char *r=onion_dict_get(ptr, k.c_str());
      if (!r)
        return def;
      return r;
    }
    
    void add(const std::string &k, const std::string &v, int flags=OD_DUP_ALL){
      onion_dict_add(ptr,k.c_str(),v.c_str(),flags);
    }
    void add(const std::string &k, Dict &v, int flags=OD_DUP_ALL){
      onion_dict_add(ptr,k.c_str(),v.c_handle(),flags|OD_DICT);
    }
    
    onion_dict *c_handle(){
      return ptr;
    }
  };
}

#endif

