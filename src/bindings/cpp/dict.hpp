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
#include <onion/log.h>
#include <onion/block.h>


namespace Onion{
  class Dict{
    onion_dict *ptr;
		bool autodelete;
  public:
    class key_not_found : public std::exception{
      std::string msg;
    public:
      key_not_found(const std::string &key) : msg("Key "+key+" not found"){}
      virtual ~key_not_found() throw(){}
      const char *what() const throw(){ return msg.c_str(); }
    };
    
    Dict() : ptr(onion_dict_new()), autodelete(true){}
    Dict(onion_dict *_ptr, bool autodelete=false) : ptr(_ptr), autodelete(autodelete){}
    Dict(const onion_dict *_ptr) : ptr((onion_dict*)_ptr), autodelete(false){} // FIXME, loses constness
    ~Dict(){
			if (autodelete)
				onion_dict_free(ptr);
		}
    
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
    
    void add(const char *k, const char *v){
			onion_dict_add(ptr, k, v, 0);
		}
    void add(const char *k, char *v){
			onion_dict_add(ptr, k, v, OD_DUP_VALUE);
		}
    void add(char *k, const char *v){
			onion_dict_add(ptr, k, v, OD_DUP_KEY);
		}
    void add(char *k, char *v){
			onion_dict_add(ptr, k, v, OD_DUP_ALL);
		}

    void add(const char *k, const Dict &v){
      onion_dict_add(ptr,k,((Dict*)&v)->c_handler(),OD_DICT);
    }
    void add(const char *k, Dict &v){
      onion_dict_add(ptr,k,v.c_handler(),OD_DUP_VALUE|OD_DICT);
    }	
    void add(char *k, Dict &v){
      onion_dict_add(ptr,k,v.c_handler(),OD_DUP_ALL|OD_DICT);
    }
    
    void add(const std::string &k, const std::string &v, int flags=OD_DUP_ALL){
      onion_dict_add(ptr,k.c_str(),v.c_str(),flags);
    }
    void add(const std::string &k, Dict &v, int flags=OD_DUP_ALL){
      onion_dict_add(ptr,k.c_str(),v.c_handler(),flags|OD_DICT);
    }
    
    void remove(const std::string &k){
			onion_dict_remove(ptr, k.c_str());
		}
    
    /// Sets the autodelete on delete flag. Use with care.
    void setAutodelete(bool s){
			autodelete=s;
		}
    
		size_t count() const{
			return onion_dict_count(ptr);
		}
		
		std::string toJSON() const{
			onion_block *bl=onion_dict_to_json(ptr);
			std::string str=onion_block_data(bl);
			onion_block_free(bl);
			return str;
		}
    
    onion_dict *c_handler(){
      return ptr;
    }
  };
}

#endif

