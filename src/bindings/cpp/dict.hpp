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

#ifndef ONION_DICT_HPP
#define ONION_DICT_HPP

#include <onion/types.h>
#include <string>
#include <onion/dict.h>
#include <onion/log.h>
#include <onion/block.h>
#include <map>

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
		
		// Slow but useful.
		Dict(const std::map<std::string, std::string> &values) : ptr(onion_dict_new()){
// 			ONION_DEBUG0("Dict %p:%p", this, ptr);
			std::map<std::string, std::string>::const_iterator I=values.begin(), endI=values.end();
			for(;I!=endI;++I){
				add(I->first, I->second);
			}
		};
    Dict() : ptr(onion_dict_new()){
// 			ONION_DEBUG0("Dict %p:%p", this, ptr);
		}
    /// WARNING Losts constness
		Dict(const onion_dict *_ptr, bool owner=false) : ptr((onion_dict*)_ptr){
// 			ONION_DEBUG0("Dict %p:%p", this, ptr);
			if (!owner)
				onion_dict_dup(ptr);
		}
		Dict(const Dict &d) : ptr(d.ptr){
			onion_dict_dup(ptr);
// 			ONION_DEBUG0("Dict %p:%p", this, ptr);
		}
		
    ~Dict(){
// 			ONION_DEBUG0("~Dict %p:%p", this, ptr);
			onion_dict_free(ptr);
		}
    
		std::string operator[](const std::string &k) const{
			const char *ret=onion_dict_get(ptr, k.c_str());
			if (!ret)
				throw(key_not_found(k));
			return onion_dict_get(ptr, k.c_str());
		}
		
		Dict &operator=(const Dict &o){
// 			ONION_DEBUG0("= %p:%p (~%p:%p)", &o, o.ptr, this, ptr);
			onion_dict *ptr2=ptr;
			ptr=onion_dict_dup((onion_dict*)(o.ptr));
			onion_dict_free(ptr2);
// 			ONION_DEBUG0("Done");
			return *this;
		}

		Dict &operator=(const onion_dict *o){
// 			ONION_DEBUG0("%p = %p (~%p)", this, o, ptr);
			onion_dict *ptr2=ptr;
			ptr=onion_dict_dup((onion_dict*)(o));
			onion_dict_free(ptr2);
			return *this;
		}

		Dict hard_dup() const{
			return Dict(onion_dict_hard_dup(ptr), true);
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
		void add(const std::string &k, const Dict &v, int flags=OD_DUP_ALL){
			onion_dict_add(ptr,k.c_str(),(const_cast<Dict*>(&v))->c_handler(),flags|OD_DICT);
		}
		
		void remove(const std::string &k){
			onion_dict_remove(ptr, k.c_str());
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
		
		operator std::map<std::string, std::string>();
		
		onion_dict *c_handler(){
			return ptr;
		}
	};
}

#endif

