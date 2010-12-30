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

#include <onion/dict.h>
#include <onion/log.h>

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

void t01_create_add_free_10(){
	INIT_LOCAL();
	onion_dict *dict;
	const char *value;
	
	dict=onion_dict_new();
	FAIL_IF_EQUAL(dict,NULL);

	// Get before anything in
	value=onion_dict_get(dict, "Request");
	FAIL_IF_NOT_EQUAL(value,NULL);

	// basic add
	int i;
	char tmp[256];
	for (i=0;i<10;i++){
		snprintf(tmp,sizeof(tmp),"%d",(i*13)%10);
		//ONION_DEBUG("add key %s",tmp);
		onion_dict_add(dict, tmp, "GET /", OD_DUP_ALL);
		value=onion_dict_get(dict, tmp);
		FAIL_IF_NOT_EQUAL_STR(value,"GET /");
		//onion_dict_print_dot(dict);
	}
	for (i=0;i<10;i++){
		snprintf(tmp,sizeof(tmp),"%d",i);
		//ONION_DEBUG("rm key %s",tmp);
		onion_dict_remove(dict, tmp);
		value=onion_dict_get(dict, tmp);
		FAIL_IF_NOT_EQUAL(value,NULL);
		//onion_dict_print_dot(dict);
	}
	
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
		
		int ok=onion_dict_remove(dict, key);
		FAIL_IF_NOT(ok);
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
	
	onion_dict_free(dict);
	
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

	onion_dict_free(dict);
	
	END_LOCAL();
}

void t04_create_and_free_a_dup(){
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

	onion_dict_add(dict, "Request", "GET /", OD_DUP_ALL);
	value=onion_dict_get(dict, "Request");
	FAIL_IF_NOT_EQUAL_STR(value,"GET /");
	
	// basic remove
	onion_dict_remove(dict, "Request");
	value=onion_dict_get(dict, "Request");
	FAIL_IF_NOT_EQUAL_STR(value,"GET /");
	
	// basic remove
	onion_dict_remove(dict, "Request");
	value=onion_dict_get(dict, "Request");
	FAIL_IF_NOT_EQUAL(value,NULL);

	onion_dict_free(dict);
	
	END_LOCAL();
}

void append_as_headers(const char *key, const char *value, void *data){
	char *str=data;
	char tmp[1024];
	sprintf(tmp,"%s: %s\n", key, value);
	strcat(str,tmp);
}


void t05_preorder(){
	INIT_LOCAL();
	onion_dict *dict;
	dict=onion_dict_new();
	
	onion_dict_add(dict,"A","B",0);
	onion_dict_add(dict,"C","D",0);
	onion_dict_add(dict,"E","F",0);
	onion_dict_add(dict,"G","H",0);
	onion_dict_add(dict,"I","J",0);
	onion_dict_add(dict,"K","L",0);
	onion_dict_add(dict,"M","N",0);
	onion_dict_add(dict,"O","P",0);
	onion_dict_add(dict,"Q","R",0);
	onion_dict_add(dict,"S","T",0);
	
	char buffer[4096];
	memset(buffer,0,sizeof(buffer));
	onion_dict_preorder(dict, append_as_headers, buffer);
	FAIL_IF_NOT_EQUAL_STR(buffer,"A: B\nC: D\nE: F\nG: H\nI: J\nK: L\nM: N\nO: P\nQ: R\nS: T\n");
	
	onion_dict_free(dict);
	END_LOCAL();
}

void t06_null_add(){
	INIT_LOCAL();
	onion_dict *dict;
	dict=onion_dict_new();
	
	onion_dict_add(dict,"b",NULL,0);
	onion_dict_add(dict,"a",NULL,0);
	onion_dict_add(dict,"c","1",0);
	
	FAIL_IF_NOT_EQUAL_STR(onion_dict_get(dict,"c"),"1");
	FAIL_IF_NOT_EQUAL(onion_dict_get(dict,"a"),NULL);
	
	onion_dict_free(dict);
	END_LOCAL();
}

int main(int argc, char **argv){
	t01_create_add_free();
	t01_create_add_free_10();
	t02_create_and_free_a_lot(100);
	t03_create_and_free_a_lot_random(100);
	t04_create_and_free_a_dup();
	t05_preorder();
	t06_null_add();
	//t07_replace(); // TODO
	
	END();
}


