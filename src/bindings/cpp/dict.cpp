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