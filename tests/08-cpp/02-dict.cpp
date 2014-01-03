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


int main(int argc, char **argv){
  START();
	INFO("Remember to check with valgrind");
	t01_basic();
	t02_dup();

	
	END();
}
