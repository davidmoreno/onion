/*
	Onion HTTP server library
	Copyright (C) 2010 David Moreno Montero

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Affero General Public License as
	published by the Free Software Foundation, either version 3 of the
	License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Affero General Public License for more details.

	You should have received a copy of the GNU Affero General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
	*/
#include <iostream>

#include "../ctest.h"

#include <bindings/cpp/onion.hpp>
#include <bindings/cpp/response.hpp>
#include <bindings/cpp/url.hpp>
#include <bindings/cpp/request.hpp>


void t01_basic(){ 
	INIT_LOCAL();
	
	Onion::Dict normal;
	normal.add("Hello", "World");
	
	onion_dict *d=onion_dict_new();
	onion_dict_add(d, "Hello", "2", 0);
	
	Onion::Dict non_owner(d);
	FAIL_IF_NOT_EQUAL_STRING(non_owner.get("Hello"), "2");
	
	Onion::Dict owner(d, true);
	FAIL_IF_NOT_EQUAL_STRING(owner.get("Hello"), "2");
	
	non_owner.add("non-owner", "true");
	FAIL_IF_NOT_EQUAL_STRING(owner.get("non-owner"), "true");
	FAIL_IF_NOT_EQUAL_STRING(onion_dict_get(d,"non-owner"), "true");
	
	END_LOCAL();
}

void t02_dup(){ 
	INIT_LOCAL();
	
	Onion::Dict normal;
	normal.add("Hello", "World");

	Onion::Dict copy4;
	copy4=normal.c_handler();
	FAIL_IF_NOT_EQUAL_STRING(copy4.get("Hello"), "World");

	Onion::Dict copy2;
	copy2=normal;
	FAIL_IF_NOT_EQUAL_STRING(copy2.get("Hello"), "World");
	
	Onion::Dict copy=normal;
	FAIL_IF_NOT_EQUAL_STRING(copy.get("Hello"), "World");

	Onion::Dict copy3(normal);
	FAIL_IF_NOT_EQUAL_STRING(copy3.get("Hello"), "World");
	
	Onion::Dict dup=normal.hard_dup();
	FAIL_IF_NOT_EQUAL_STRING(dup.get("Hello"), "World");
	
	dup.add("Hello","world!",OD_REPLACE|OD_DUP_ALL);
	FAIL_IF_EQUAL_STRING(dup.get("Hello"), "World");
	FAIL_IF_NOT_EQUAL_STRING(dup.get("Hello"), "world!");

	FAIL_IF_EQUAL_STRING(copy.get("Hello"), "world!");
	FAIL_IF_EQUAL_STRING(copy2.get("Hello"), "world!");

	normal.add("Tst","tst");
	FAIL_IF_NOT_EQUAL_STRING(copy.get("Tst"), "tst");
	FAIL_IF_NOT_EQUAL_STRING(copy2.get("Tst"), "tst");
	FAIL_IF_EQUAL_STRING(dup.get("Tst"), "tst");
	
	END_LOCAL();
}


void t03_subdict(){
	INIT_LOCAL();
	
	Onion::Dict a;
	a.add("Hello", "World");
	{
#if __cplusplus >= 201103L
		// C++11 Style add, if not using c++11, compile anyway.
		Onion::Dict b( {{"Hello","World"},{"Another","item"}} );
#else
		Onion::Dict b;
		b.add("Hello","World");
		b.add("Another","item");
#endif
		a.add("dict",b);
	}
	
	std::string json=a.toJSON();
	std::cout<<json<<std::endl;;
	
	FAIL_IF_NOT_EQUAL_STRING(json, "{\"Hello\":\"World\", \"dict\":{\"Another\":\"item\", \"Hello\":\"World\"}}");
	
	END_LOCAL();
};

const Onion::Dict getLanguagesDict(){
	const char *languages[2][2]={ 
		{"en", "English"},
		{"es", "Español"}
	};

	Onion::Dict ret;
	ret.add(languages[0][0], languages[0][1]);
	ret.add(languages[1][0], languages[1][1]);
	
	std::string json=ret.toJSON();
	std::cout<<json<<std::endl;;
	
	return ret;
}

void t04_langtest(){
	INIT_LOCAL();
	Onion::Dict main_dict;
	/*  == With bug do not fail with this workaround: ==
	const Onion::Dict test = getLanguagesDict();
	main_dict.add("languages", test);
	*/
	main_dict.add("languages", getLanguagesDict());
	std::string json=main_dict.toJSON();
	std::cout<<json<<std::endl;;

	Onion::Dict languages = main_dict.getDict("languages");
	json=main_dict.toJSON();
	std::cout<<json<<std::endl;;

	FAIL_IF_NOT_EQUAL_STRING(languages.get("en"), "English");
	FAIL_IF_NOT_EQUAL_STRING(languages.get("es"), "Español");

	
	END_LOCAL();
}

onion_connection_status index_html_template(onion_dict *context){
	char *lang = (char*)malloc (3*sizeof(char));
	strcpy(lang,"en\0");  
	ONION_DEBUG("Add to dict");
  if (context) onion_dict_add(context, "LANG", lang, OD_FREE_VALUE);
	ONION_DEBUG("Free dict");
  if (context) onion_dict_free(context); // if you remove this line, it works correctly
  return OCS_PROCESSED;
}


Onion::Dict defaultContext(Onion::Dict dict_){
  dict_.add("serial", "serial"); 
  return dict_;
}

Onion::Dict defaultContext(){
  Onion::Dict dict; 
  return defaultContext(dict);
}

void t05_context(){
	INIT_LOCAL();
	Onion::Dict dict(defaultContext());
	ONION_DEBUG("Render");
	
	// This is important as otemplate templates need ownership as they 
	// will free the pointer. These way we use the internal refcount to avoid double free.
	onion_dict_dup(dict.c_handler()); 
	
	index_html_template(dict.c_handler());
	END_LOCAL();
}


void t06_tomap(){
	INIT_LOCAL();
	
#if __cplusplus > 199711L
	std::map<std::string, std::string> orig{{"Hello","World"}};
#else
	std::map<std::string, std::string> orig;
	orig.insert(std::pair< std::string, std::string>("Hello","World"));
#endif
	Onion::Dict d(orig);
	
	std::map<std::string, std::string> dup=d;
	
	FAIL_IF_NOT_EQUAL_STRING(dup["Hello"],"World");
	
	END_LOCAL();
}

void t07_from_initializer_list(){
	INIT_LOCAL();
#if __cplusplus > 199711L
	Onion::Dict b;
	b.add("test", {{"test", "init by initializer list"},{"test2", "Should work"}});
	
	FAIL_IF_NOT_EQUAL_STRING(b.getDict("test").get("test"), "init by initializer list");
	FAIL_IF_NOT_EQUAL_STRING(b.getDict("test").get("test2"), "Should work");
#endif
	END_LOCAL();
}

int main(int argc, char **argv){
  START();
	INFO("Remember to check with valgrind");
	t01_basic();
	t02_dup();
	t03_subdict();
	t04_langtest();
	t05_context();
	t06_tomap();
	t07_from_initializer_list();
	
	END();
}
