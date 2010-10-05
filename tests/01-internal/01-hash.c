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

#include <malloc.h>
#include <string.h>

#include <onion_dict.h>

#include "../test.h"


void t01_create_add_free(){
	INIT_LOCAL();
	onion_dict *dict;
	const char *value;
	
	dict=onion_dict_new();
	FAIL_IF_EQUAL(dict,NULL);

	// Get before anything in
	value=onion_dict_get(dict, "Request");
	FAIL_IF_NOT_EQUAL(value,NULL);

	// basic add
	onion_dict_add(dict, "Request", "GET /", OD_DUP_ALL);
	value=onion_dict_get(dict, "Request");
	FAIL_IF_NOT_EQUAL_STR(value,"GET /");
	
	// basic remove
	onion_dict_remove(dict, "Request");
	value=onion_dict_get(dict, "Request");
	FAIL_IF_NOT_EQUAL(value,NULL);

	onion_dict_free(dict);
	
	END_LOCAL();
}

void t02_create_and_free_a_lot(unsigned int n){
	INIT_LOCAL();
	onion_dict *dict;
	const char *value;
	unsigned int i;
	
	dict=onion_dict_new();
	FAIL_IF_EQUAL(dict,NULL);

	// Linear add
	for (i=0;i<n;i++){
		char key[16], val[16];
		sprintf(key,"key %d",i);
		sprintf(val,"val %d",i);
		
		onion_dict_add(dict, key, val, OD_DUP_ALL);
	}

	// Linear get
	for (i=0;i<n;i++){
		char key[16], val[16];
		sprintf(key,"key %d",i);
		sprintf(val,"val %d",i);
		
		value=onion_dict_get(dict, key);
		FAIL_IF_NOT_EQUAL_STR(val,value);
	}

	// remove all
	for (i=0;i<n;i++){
		char key[16];
		sprintf(key,"key %d",i);
		
		onion_dict_remove(dict, key);
	}

	// check removed all
	for (i=0;i<n;i++){
		char key[16], val[16];
		sprintf(key,"key %d",i);
		sprintf(val,"val %d",i);
		
		value=onion_dict_get(dict, key);
		//fprintf(stderr,"%s %s\n",key,value);
		FAIL_IF_NOT_EQUAL(NULL,value);
		FAIL_IF_NOT_EQUAL_STR(NULL,value);
	}
	
	END_LOCAL();
}


#define R1(x) ((x)*39872265)%28645
#define R2(x) ((x)*43433422)%547236

void t03_create_and_free_a_lot_random(unsigned int n){
	INIT_LOCAL();
	onion_dict *dict;
	const char *value;
	unsigned int i;
	
	dict=onion_dict_new();
	FAIL_IF_EQUAL(dict,NULL);

	// Linear add
	for (i=0;i<n;i++){
		char key[16], val[16];
		sprintf(key,"key %d",R1(i));
		sprintf(val,"val %d",R2(i));
		
		onion_dict_add(dict, key, val, OD_DUP_ALL);
	}

	// Linear get
	for (i=0;i<n;i++){
		char key[16], val[16];
		sprintf(key,"key %d",R1(i));
		sprintf(val,"val %d",R2(i));
		
		value=onion_dict_get(dict, key);
		FAIL_IF_NOT_EQUAL_STR(val,value);
	}

	// remove all
	for (i=n;i>0;i--){
		char key[16];
		int removed;
		sprintf(key,"key %d",R1(i-1));
		//fprintf(stderr,"%s %d\n",key,i-1);
		removed=onion_dict_remove(dict, key);
		FAIL_IF_NOT_EQUAL(removed,1);
	}

	// check removed all
	for (i=0;i<n;i++){
		char key[16], val[16];
		sprintf(key,"key %d",R1(i));
		sprintf(val,"val %d",R1(i));
		
		value=onion_dict_get(dict, key);
		//fprintf(stderr,"%s %s\n",key,value);
		FAIL_IF_NOT_EQUAL(NULL,value);
		FAIL_IF_NOT_EQUAL_STR(NULL,value);
	}
	
	END_LOCAL();
}

int main(int argc, char **argv){
	t01_create_add_free();
	t02_create_and_free_a_lot(100);
	t03_create_and_free_a_lot_random(100);
	
	END();
}


