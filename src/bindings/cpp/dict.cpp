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

#include "dict.hpp"
#include <map>

Onion::Dict::key_not_found::key_not_found(const std::string& key)
	: msg("Key " + key + " not found")
{}

Onion::Dict::key_not_found::~key_not_found() noexcept
{}

const char* Onion::Dict::key_not_found::what() const noexcept
{
	return msg.c_str();
}


Onion::Dict::Dict(const std::map<std::string, std::string>& values)
	: ptr { onion_dict_new(), &onion_dict_free }
{
	for(const auto& pair : values)
		add(pair.first, pair.second);
}

Onion::Dict::Dict(std::initializer_list<std::initializer_list<std::string>> &&init)
	: ptr { onion_dict_new(), &onion_dict_free }
{
	for(auto p : init)
	{
		auto I = p.begin();
		auto k = *I;
		++I;
		auto v = *I;
		add(k, v);
	}
}

Onion::Dict::Dict(const onion_dict *_ptr, bool owner)
	: ptr { nullptr, [](onion_dict*) -> void {}  }
{
	if(!owner)
		ptr = internal_pointer { const_cast<onion_dict*>(_ptr), [](onion_dict*) -> void {} };
	else
		ptr = internal_pointer { const_cast<onion_dict*>(_ptr), &onion_dict_free };
}

Onion::Dict::Dict(const Dict& d)
	: ptr { d.ptr.get(), &onion_dict_free }
{
	onion_dict_dup(ptr.get());
}

Onion::Dict& Onion::Dict::operator=(const Dict &o)
{
	ptr = internal_pointer { o.ptr.get(), &onion_dict_free };
	onion_dict_dup(o.ptr.get());
	return *this;
}

Onion::Dict& Onion::Dict::operator=(onion_dict* o)
{
	ptr = internal_pointer { o, &onion_dict_free };
	onion_dict_dup(ptr.get());
	return *this;
}

Onion::Dict Onion::Dict::hard_dup() const
{
	return Dict(onion_dict_hard_dup(ptr.get()), true);
}

bool Onion::Dict::has(const std::string& k) const noexcept
{
	return onion_dict_get(ptr.get(), k.c_str()) != nullptr;
}

Onion::Dict Onion::Dict::getDict(const std::string& k) const noexcept
{
	return Dict(onion_dict_get_dict(ptr.get(), k.c_str()));
}

Onion::Dict& Onion::Dict::add(const std::string &k, const std::string &v, int flags) noexcept
{
	onion_dict_add(ptr.get(), k.c_str(), v.c_str(), flags);
	return *this;
}

Onion::Dict& Onion::Dict::add(const std::string &k, const Dict& v, int flags) noexcept
{
	onion_dict_add(ptr.get(), k.c_str(), v.ptr.get(), flags | OD_DICT);
	return *this;
}

Onion::Dict& Onion::Dict::remove(const std::string& k) noexcept
{
	onion_dict_remove(ptr.get(), k.c_str());
	return *this;
}

std::string Onion::Dict::toJSON() const noexcept
{
	onion_block *bl = onion_dict_to_json(ptr.get());
	std::string str = onion_block_data(bl);
	onion_block_free(bl);
	return str;
}

Onion::Dict Onion::Dict::fromJSON(const std::string &jsondata) noexcept
{
	return Dict(onion_dict_from_json(jsondata.c_str()));
}

Onion::Dict& Onion::Dict::merge(const Dict &other) noexcept
{
	onion_dict_merge(ptr.get(), other.ptr.get());
	return *this;
}

onion_dict* Onion::Dict::c_handler() const noexcept
{
	return ptr.get();
}

namespace Onion{
	void add_to_map(std::map<std::string, std::string> *ret, const char *key, const char *value, int flags){
		if ((flags & OD_DICT)==0)
			(*ret)[key]=value;
	}
	
	Dict::operator std::map<std::string, std::string>(){
		std::map<std::string, std::string> ret;
		
		onion_dict_preorder(c_handler(), (void*)add_to_map, (void*)&ret);
		
		return ret;
	}
};
