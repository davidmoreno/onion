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
	/**
	 * @short Wrapper for onion_dict, manages hierachical dictionaries.
	 * 
	 * WARNING: Its a shared dict, any opy is really a reference to the same dict. When all 
	 *  dicts are freed, the memory is finally freed.
	 * 
	 * This data structure is usefull to interface with onion directly, and should support as 
	 * many C++ idioms as possible, but std::map is a better data structure for you own code.
	 * 
	 * There are options to copy to and from std::map, but limitations apply (no subdicts), and
	 * it ensures copy of data and more memory use.
	 */
	class Dict{
		onion_dict *ptr;
	public:
		/**
		 * @short The asked key does not exist.
		 */
		class key_not_found : public std::exception{
			std::string msg;
		public:
			key_not_found(const std::string &key) : msg("Key "+key+" not found"){}
			virtual ~key_not_found() throw(){}
			const char *what() const throw(){ return msg.c_str(); }
		};
		
		/**
		 * @short Converts from a std::map<string,string> to a Dict. It copies all values.
		 */
		Dict(const std::map<std::string, std::string> &values) : ptr(onion_dict_new()){
// 			ONION_DEBUG0("Dict %p:%p", this, ptr);
			std::map<std::string, std::string>::const_iterator I=values.begin(), endI=values.end();
			for(;I!=endI;++I){
				add(I->first, I->second);
			}
		};
		/**
		 * @short Creates an empty Onion::Dict
		 */
    Dict() : ptr(onion_dict_new()){
// 			ONION_DEBUG0("Dict %p:%p", this, ptr);
		}
    /**
		 * @short Creates a Onion::Dict from a c onion_dict.
		 * 
		 * WARNING Losts constness. I found no way to keep it.
		 */
		Dict(const onion_dict *_ptr, bool owner=false) : ptr((onion_dict*)_ptr){
// 			ONION_DEBUG0("Dict %p:%p", this, ptr);
			if (!owner)
				onion_dict_dup(ptr);
		}
		/**
		 * @short Creates a new reference to the dict.
		 * 
		 * WARNING: As Stated it is a reference, not a copy. For that see Onion::Dict::hard_dup.
		 */
		Dict(const Dict &d) : ptr(d.ptr){
			onion_dict_dup(ptr);
// 			ONION_DEBUG0("Dict %p:%p", this, ptr);
		}
		
		/**
		 * @short Frees this reference.
		 */
    ~Dict(){
// 			ONION_DEBUG0("~Dict %p:%p", this, ptr);
			onion_dict_free(ptr);
		}
    
    /**
		 * @short Try to get a key, if not existant, throws an key_not_found exception.
		 */
		std::string operator[](const std::string &k) const{
			const char *ret=onion_dict_get(ptr, k.c_str());
			if (!ret)
				throw(key_not_found(k));
			return onion_dict_get(ptr, k.c_str());
		}
		
		/**
		 * @short Assings the reference to the dictionary.
		 */
		Dict &operator=(const Dict &o){
// 			ONION_DEBUG0("= %p:%p (~%p:%p)", &o, o.ptr, this, ptr);
			onion_dict *ptr2=ptr;
			ptr=onion_dict_dup((onion_dict*)(o.ptr));
			onion_dict_free(ptr2);
// 			ONION_DEBUG0("Done");
			return *this;
		}

		/**
		 * @short Assigns the reference to the ditionary, from a C onion_dict.
		 */
		Dict &operator=(const onion_dict *o){
// 			ONION_DEBUG0("%p = %p (~%p)", this, o, ptr);
			onion_dict *ptr2=ptr;
			ptr=onion_dict_dup((onion_dict*)(o));
			onion_dict_free(ptr2);
			return *this;
		}

		/**
		 * @short Creates a real copy of all the elements on the dictionary.
		 */
		Dict hard_dup() const{
			return Dict(onion_dict_hard_dup(ptr), true);
		}
		
		/**
		 * @short Checks if an element is contained on the dictionary. 
		 * 
		 * It do a get internally, so if after this there is a get, a get 
		 * with a default value is more efficient, or depending on your case, 
		 * a try catch block.
		 */
		bool has(const std::string &k) const{
			return onion_dict_get(ptr, k.c_str())!=NULL;
		}
		
		/**
		 * @short Returns a subdictionary.
		 * 
		 * For a given key, returns the subdictionary.
		 */
		Dict getDict(const std::string &k) const{
			return Dict(onion_dict_get_dict(ptr, k.c_str()));
		}
		
		/**
		 * @short Returns a value on the dictionary, or a default value.
		 * 
		 * For the given key, if possible returns the value, and if its 
		 * not contained, returns a default value.
		 */
		std::string get(const std::string &k, const std::string &def="") const{
			const char *r=onion_dict_get(ptr, k.c_str());
			if (!r)
				return def;
			return r;
		}
		
		/**
		 * @short Adds a string value to the dictionary.
		 * 
		 * By defaults do a copy of everything, but can be tweaked. If user plans to do
		 * low level onion_dict adding, its better to use the C onion_dict_add function.
		 */
		void add(const std::string &k, const std::string &v, int flags=OD_DUP_ALL){
			onion_dict_add(ptr,k.c_str(),v.c_str(),flags);
		}
		
		/**
		 * @short Adds a subdictionary to this dictionary.
		 */
		void add(const std::string &k, const Dict &v, int flags=OD_DUP_ALL){
			onion_dict_add(ptr,k.c_str(),(const_cast<Dict*>(&v))->c_handler(),flags|OD_DICT);
		}
		
		/**
		 * @short Removes an element from the dictionary.
		 */
		void remove(const std::string &k){
			onion_dict_remove(ptr, k.c_str());
		}
		
		/**
		 * @short Count of elements on the dictionary.
		 */
		size_t count() const{
			return onion_dict_count(ptr);
		}
		
		/**
		 * @short Converts the dictionary to a JSON string.
		 */
		std::string toJSON() const{
			onion_block *bl=onion_dict_to_json(ptr);
			std::string str=onion_block_data(bl);
			onion_block_free(bl);
			return str;
		}
		
		/**
		 * @short Converts the dictionary to a std::map<string,string>.
		 * 
		 * If there are subdictionaries, they are ignored.
		 */
		operator std::map<std::string, std::string>();
		
		/**
		 * @short Returns the C onion_dict handler, to be able to use C functions.
		 */
		onion_dict *c_handler(){
			return ptr;
		}
	};
}

#endif

