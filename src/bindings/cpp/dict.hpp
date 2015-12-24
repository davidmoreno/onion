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
#include <memory>

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
		using internal_pointer = std::unique_ptr<onion_dict, decltype(onion_dict_free)*>;
		internal_pointer ptr;

	public:
		/**
		 * @short The asked key does not exist.
		 */
		class key_not_found : public std::exception
		{
		public:
			key_not_found(const std::string& key);
			virtual ~key_not_found() noexcept;
			const char* what() const noexcept;
		private:
		std::string msg;
		};

		/**
		 * @short Represents a lock on a Dict instance.
		 */
		class lock
		{
		public:
			/**
			 * @short Create a lock object from a Dict.
			 */
			explicit lock(onion_dict* d) : dict(d) { }
			lock(lock&& l) : dict(l.dict) { l.dict = nullptr; }

			/**
			 * @short Automatically releases a lock when the scope is exited.
			 */
			~lock() { onion_dict_unlock(dict); }
		private:
			onion_dict* dict;
		};
		
		/**
		 * @short Converts from a std::map<string,string> to a Dict. It copies all values.
		 */
		Dict(const std::map<std::string, std::string> &values);

		/**
		 * @short Created directly from initializer list. Only for > C++11
		 * 
		 * Example
		 * @code
		 * 	Onion::Dict dict{{"hello","world"},{"this","is a test"}}
		 * @endcode
		 */
		Dict(std::initializer_list<std::initializer_list<std::string>> &&init);

		/**
		 * @short Creates an empty Onion::Dict
		 */
	    Dict() : ptr { onion_dict_new(), &onion_dict_free } {}

		/**
		 * @short Creates a Onion::Dict from a c onion_dict.
		 * 
		 * WARNING Losts constness. I found no way to keep it.
		 */
        explicit Dict(const onion_dict *_ptr, bool owner = false);

		/**
		 * @short Creates a new reference to the dict.
		 * 
		 * WARNING: As Stated it is a reference, not a copy. For that see Onion::Dict::hard_dup.
		 */
		Dict(const Dict &d);
		
		/**
		 * @short Frees this reference.
		 */
	    ~Dict() {}
    
	    /**
		 * @short Try to get a key, if not existant, throws an key_not_found exception.
		 */
		std::string operator[](const std::string &k) const
		{
			const char* r = onion_dict_get(ptr.get(), k.c_str());
			if(!r)
				throw key_not_found { k };
			return r;
		}
		
		/**
		 * @short Assings the reference to the dictionary.
		 */
		Dict &operator=(const Dict &o);

		/**
		 * @short Assigns the reference to the ditionary, from a C onion_dict.
		 */
		Dict &operator=(onion_dict *o);

		/**
		 * @short Creates a real copy of all the elements on the dictionary.
		 */
		Dict hard_dup() const;
		
		/**
		 * @short Checks if an element is contained on the dictionary. 
		 * 
		 * It do a get internally, so if after this there is a get, a get 
		 * with a default value is more efficient, or depending on your case, 
		 * a try catch block.
		 */
		bool has(const std::string &k) const noexcept;
		
		/**
		 * @short Returns a subdictionary.
		 * 
		 * For a given key, returns the subdictionary.
		 */
		Dict getDict(const std::string &k) const noexcept;
		
		/**
		 * @short Returns a value on the dictionary, or a default value.
		 * 
		 * For the given key, if possible returns the value, and if its 
		 * not contained, returns a default value.
		 */
		std::string get(const std::string &k, const std::string &def="") const noexcept
		{
			const char *r = onion_dict_get(ptr.get(), k.c_str());
			return (!r) ? def : r;
		}
		
		/**
		 * @short Adds a string value to the dictionary.
		 * 
		 * By defaults do a copy of everything, but can be tweaked. If user plans to do
		 * low level onion_dict adding, its better to use the C onion_dict_add function.
		 */
		Dict &add(const std::string &k, const std::string &v, int flags=OD_DUP_ALL) noexcept;
		
		/**
		 * @short Adds a subdictionary to this dictionary.
		 */
		Dict &add(const std::string &k, const Dict &v, int flags=OD_DUP_ALL) noexcept;
		
		/**
		 * @short Removes an element from the dictionary.
		 */
		Dict &remove(const std::string &k) noexcept;
		
		/**
		 * @short Count of elements on the dictionary.
		 */
		size_t count() const noexcept
		{
			return onion_dict_count(ptr.get());
		}
		
		/**
		 * @short Converts the dictionary to a JSON string.
		 */
		std::string toJSON() const noexcept;
		
		/**
		 * @short Convert a JSON string to an onion dictionary. 
		 * 
		 * This is not full JSON support, only dict side, and with strings.
		 */
		static Dict fromJSON(const std::string &jsondata) noexcept;
		
		/**
		 * @short Merges argument dict into current.
		 * 
		 * If there are repeated keys, they will repeated.
		 */
		Dict &merge(const ::Onion::Dict &other) noexcept;
		
		/**
		 * @short Converts the dictionary to a std::map<string,string>.
		 * 
		 * If there are subdictionaries, they are ignored.
		 */
		operator std::map<std::string, std::string>();

		/**
		 * @short Get a read lock on this Dict
		 *
		 * The lock is released automatically when the scope is exited. An example of usage:
		 * @code
		 * Onion::Dict d;
		 *
		 *	{
		 *	  auto lock = d.getReadLock();
		 *	  d.get("test");
		 *	}
		 * //The lock should be released by this point.
		 * @endcode
		 */
		lock readLock() const noexcept
		{
			onion_dict_lock_read(ptr.get());
			return lock { ptr.get() };
		}

		/**
		 * @short Get a write lock on this Dict
		 *
		 * The lock is released automatically when the scope is exited.
		 */
		lock writeLock() const noexcept
		{
			onion_dict_lock_write(ptr.get());
			return lock { ptr.get() };
		}
		
		/**
		 * @short Returns the C onion_dict handler, to be able to use C functions.
		 */
		onion_dict *c_handler() const noexcept;
	};
}

#endif

