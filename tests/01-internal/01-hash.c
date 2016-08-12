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

#include <stdlib.h>
#include <string.h>

#include <onion/dict.h>
#include <onion/log.h>

#include "../ctest.h"
#include <unistd.h>
#include <onion/block.h>

#ifdef HAVE_PTHREADS
#include <pthread.h>
#endif


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
		////ONION_DEBUG("add key %s",tmp);
		onion_dict_add(dict, tmp, "GET /", OD_DUP_ALL);
		value=onion_dict_get(dict, tmp);
		FAIL_IF_NOT_EQUAL_STR(value,"GET /");
		//onion_dict_print_dot(dict);
	}
	for (i=0;i<10;i++){
		snprintf(tmp,sizeof(tmp),"%d",i);
		////ONION_DEBUG("rm key %s",tmp);
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

void append_as_headers(char *str, const char *key, const char *value, int flags){
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

void t07_sum(int *v, const char *key, const char *value, int flags){
	*v+=atoi(value);
}

void t07_replace(){
	INIT_LOCAL();
	onion_dict *dict=onion_dict_new();

	onion_dict_add(dict,"a","1", OD_DUP_ALL|OD_REPLACE);
	onion_dict_add(dict,"a","1", OD_REPLACE);
	onion_dict_add(dict,"a","1", OD_DUP_ALL|OD_REPLACE);
	onion_dict_add(dict,"a","1", OD_REPLACE);
	onion_dict_add(dict,"a","1", OD_DUP_ALL|OD_REPLACE);

	int n=0;
	onion_dict_preorder(dict, t07_sum, &n);
	FAIL_IF_NOT_EQUAL_INT(n,1);

	onion_dict_add(dict,"a","1", 0);
	n=0;
	onion_dict_preorder(dict, t07_sum, &n);
	FAIL_IF_NOT_EQUAL_INT(n,2);

	onion_dict_free(dict);

	END_LOCAL();
}

#ifdef HAVE_PTHREADS
#define N_READERS 30

char *t08_thread_read(onion_dict *d){
	char done=0;
	char *ret=NULL;
	while (!done){
		//ONION_DEBUG("Lock read");
		onion_dict_lock_write(d);
		//ONION_DEBUG("Got read lock");
		const char *test=onion_dict_get(d,"test");
		if (test){
			//ONION_DEBUG("Unlock");

			//onion_dict_lock_write(d);
			//ONION_DEBUG("Got write lock");
			char tmp[16];
			snprintf(tmp,16,"%d",onion_dict_count(d));
			onion_dict_remove(d,"test");
			onion_dict_add(d,tmp,"test",OD_DUP_ALL);
			ONION_DEBUG("Write answer %d", onion_dict_count(d));
			done=1;
			//ONION_DEBUG("Unlock");
			onion_dict_unlock(d);
			ret=(char*)1;
			break;
		}
		//ONION_DEBUG("Unlock");
		onion_dict_unlock(d);
		usleep(200);
	}
	//ONION_DEBUG("dict free");
	onion_dict_free(d);
	return ret;
}

void *t08_thread_write(onion_dict *d){
	int n=0;
	while (n!=N_READERS){
		int i;
		n=0;
		//ONION_DEBUG("Lock read");
		onion_dict_lock_read(d);
		//ONION_DEBUG("Got read lock");
		for (i=0;i<N_READERS;i++){
			char tmp[16];
			snprintf(tmp,16,"%d",i+1);
			const char *r=onion_dict_get(d,tmp);
			if (r)
				n++;
		}
		//ONION_DEBUG("Unlock");
		onion_dict_unlock(d);
		//ONION_DEBUG("Lock write");
		onion_dict_lock_write(d);
		//ONION_DEBUG("Got write lock");
		onion_dict_add(d, "test", "test", OD_DUP_ALL|OD_REPLACE);
		//ONION_DEBUG("Unlock");
		onion_dict_unlock(d);
		ONION_DEBUG("Found %d answers, should be %d.", n, N_READERS);
		usleep(200);
	}

	onion_dict_free(d);
	return (char*)1;
}

void t08_threaded_lock(){
	INIT_LOCAL();

	onion_dict *d=onion_dict_new();

	pthread_t thread[N_READERS];
	int i;
	for (i=0;i<N_READERS;i++){
		onion_dict *d2=onion_dict_dup(d);
		pthread_create(&thread[i], NULL, (void*)t08_thread_read, d2);
	}
	//sleep(1);
	t08_thread_write(d);

	for (i=0;i<N_READERS;i++){
		char *v;
		pthread_join(thread[i],(void**) &v);
		FAIL_IF_NOT_EQUAL(v, (char *)v);
	}

	END_LOCAL();
}

#define NWAR 100
#define WARLOOPS 1000

void t09_thread_war_thread(onion_dict *d){
	int i;
	char tmp[16];
	for (i=0;i<WARLOOPS;i++){
		snprintf(tmp,16,"%04X",i);
		if (rand()%1){
			onion_dict_lock_read(d);
			onion_dict_get(d,tmp);
			onion_dict_unlock(d);
		}
		else{
			onion_dict_lock_write(d);
			onion_dict_add(d,tmp,tmp,OD_DUP_ALL|OD_REPLACE);
			onion_dict_unlock(d);
		}
	}
	onion_dict_free(d);
}

void t09_thread_war(){
	INIT_LOCAL();

	pthread_t thread[NWAR];
	int i;
	onion_dict *d=onion_dict_new();
	for (i=0;i<NWAR;i++){
		onion_dict *dup=onion_dict_dup(d);
		pthread_create(&thread[i], NULL, (void*)&t09_thread_war_thread, dup);
	}

	onion_dict_free(d);

	for (i=0;i<NWAR;i++){
		pthread_join(thread[i], NULL);
	}

	END_LOCAL();
}

#endif

void t10_tojson(){
	INIT_LOCAL();

	onion_dict *d=onion_dict_new();
	const char *tmp;
	int s;
	onion_block *b;
	b=onion_dict_to_json(d);
	tmp=onion_block_data(b);
	ONION_DEBUG("Json returned is '%s'", tmp);
	FAIL_IF_NOT_EQUAL_STR(tmp,"{}");
	onion_block_free(b);

	onion_dict_add(d, "test", "json", 0);

	b=onion_dict_to_json(d);
	FAIL_IF_EQUAL(b,NULL);
	tmp=onion_block_data(b);
	s=onion_block_size(b);
	ONION_DEBUG("Json returned is '%s'", tmp);
	FAIL_IF(s<=0);
	FAIL_IF_EQUAL(strstr(tmp,"{"), NULL);
	FAIL_IF_EQUAL(strstr(tmp,"}"), NULL);

	FAIL_IF_EQUAL(strstr(tmp,"\"test\""), NULL);
	FAIL_IF_EQUAL(strstr(tmp,"\"json\""), NULL);
	FAIL_IF_NOT_EQUAL(strstr(tmp,","), NULL);
	onion_block_free(b);

	onion_dict_add(d, "other", "data", 0);

	b=onion_dict_to_json(d);
	tmp=onion_block_data(b);
	s=onion_block_size(b);
	ONION_DEBUG("Json returned is '%s'", tmp);
	FAIL_IF(s<=0);
	FAIL_IF_EQUAL(strstr(tmp,"{"), NULL);
	FAIL_IF_EQUAL(strstr(tmp,"}"), NULL);

	FAIL_IF_EQUAL(strstr(tmp,"\"test\""), NULL);
	FAIL_IF_EQUAL(strstr(tmp,"\"json\""), NULL);
	FAIL_IF_EQUAL(strstr(tmp,","), NULL);
	FAIL_IF_EQUAL(strstr(tmp,"\"other\""), NULL);
	FAIL_IF_EQUAL(strstr(tmp,"\"data\""), NULL);
	onion_block_free(b);

	onion_dict_add(d, "with\"", "data\n", 0);

	b=onion_dict_to_json(d);
	tmp=onion_block_data(b);
	s=onion_block_size(b);
	ONION_DEBUG("Json returned is '%s'", tmp);
	FAIL_IF(s<=0);

	FAIL_IF_EQUAL(strstr(tmp,"\\n"), NULL);
	FAIL_IF_EQUAL(strstr(tmp,"\\\""), NULL);
	onion_block_free(b);

	onion_dict_free(d);

	END_LOCAL();
}

void cmpdict(onion_dict *d, const char *key, const char *value, int flags){
	if (flags&OD_DICT){
		onion_dict_preorder((onion_dict*)value, cmpdict, onion_dict_get_dict(d, key));
		onion_dict_preorder(onion_dict_get_dict(d, key), cmpdict, (onion_dict*)value);
	}
	else
		FAIL_IF_NOT_EQUAL_STR(value, onion_dict_get(d, key));
}

void t11_hard_dup(){
	INIT_LOCAL();

	onion_dict *orig=onion_dict_new();

	char tmp[9];
	int i;
	for (i=0;i<256;i++){
		sprintf(tmp,"%08X",rand());
		onion_dict_add(orig, tmp, tmp, OD_DUP_ALL);
	}
	onion_dict_add(orig, "0", "no frees", 0);

	onion_dict *dest=onion_dict_hard_dup(orig);

	/// Check they have exactly the same keys.
	onion_dict_preorder(orig, cmpdict, dest);
	onion_dict_preorder(dest, cmpdict, orig);

	onion_dict_free(orig);
	onion_dict_free(dest);

	END_LOCAL();
}

void t12_dict_in_dict(){
	INIT_LOCAL();

	onion_dict *A=onion_dict_new();
	onion_dict *B=onion_dict_new();
	onion_dict *C=onion_dict_new();
	onion_dict *D=onion_dict_new();

	int i;
	for (i=0;i<16;i++){
		char tmp[9];
		sprintf(tmp,"%08X",rand());
		onion_dict_add(A, tmp, tmp, OD_DUP_ALL);
		sprintf(tmp,"%08X",rand());
		onion_dict_add(B, tmp, tmp, OD_DUP_ALL);
		sprintf(tmp,"%08X",rand());
		onion_dict_add(C, tmp, tmp, OD_DUP_ALL);
		sprintf(tmp,"%08X",rand());
		onion_dict_add(D, tmp, tmp, OD_DUP_ALL);
	}

	onion_dict_add(A, "B", B, OD_DICT|OD_FREE_VALUE);
	onion_dict_add(A, "C", C, OD_DICT|OD_FREE_VALUE);
	onion_dict_add(A, "D", D, OD_DICT|OD_FREE_VALUE);

	FAIL_IF_NOT_EQUAL((onion_dict*)onion_dict_get(A, "B"), NULL);
	FAIL_IF_NOT_EQUAL((onion_dict*)onion_dict_get(A, "C"), NULL);
	FAIL_IF_NOT_EQUAL((onion_dict*)onion_dict_get(A, "D"), NULL);

	FAIL_IF_NOT_EQUAL((onion_dict*)onion_dict_get_dict(A, "B"), B);
	FAIL_IF_NOT_EQUAL((onion_dict*)onion_dict_get_dict(A, "C"), C);
	FAIL_IF_NOT_EQUAL((onion_dict*)onion_dict_get_dict(A, "D"), D);

	{
		onion_block *tmpA=onion_dict_to_json(A);
		onion_block *tmpB=onion_dict_to_json(B);
		onion_block *tmpC=onion_dict_to_json(C);
		onion_block *tmpD=onion_dict_to_json(D);
		/*
		ONION_DEBUG("Json is: %s",tmpA);
		ONION_DEBUG("Json is: %s",tmpB);
		ONION_DEBUG("Json is: %s",tmpC);
		ONION_DEBUG("Json is: %s",tmpD);
		*/
		FAIL_IF_EQUAL( strstr(onion_block_data(tmpA), onion_block_data(tmpB)), NULL);
		FAIL_IF_EQUAL( strstr(onion_block_data(tmpA), onion_block_data(tmpC)), NULL);
		FAIL_IF_EQUAL( strstr(onion_block_data(tmpA), onion_block_data(tmpD)), NULL);
		onion_block_free(tmpA);
		onion_block_free(tmpB);
		onion_block_free(tmpC);
		onion_block_free(tmpD);
	}
	B=onion_dict_hard_dup(A);

	onion_dict_free(A);
	onion_dict_free(B);

	END_LOCAL();
}

void t13_dict_rget(){
	INIT_LOCAL();

	onion_dict *A=onion_dict_new();
	onion_dict *B=onion_dict_new();
	onion_dict *C=onion_dict_new();
	onion_dict *D=onion_dict_new();

	int i;
	for (i=0;i<16;i++){
		char tmp[9];
		sprintf(tmp,"%08X",rand());
		onion_dict_add(A, tmp, tmp, OD_DUP_ALL);
		sprintf(tmp,"%08X",rand());
		onion_dict_add(B, tmp, tmp, OD_DUP_ALL);
		sprintf(tmp,"%08X",rand());
		onion_dict_add(C, tmp, tmp, OD_DUP_ALL);
		sprintf(tmp,"%08X",rand());
		onion_dict_add(D, tmp, tmp, OD_DUP_ALL);
	}

	onion_dict_add(A, "B", B, OD_DICT|OD_FREE_VALUE);
	onion_dict_add(A, "C", C, OD_DICT|OD_FREE_VALUE);
	onion_dict_add(A, "D", D, OD_DICT|OD_FREE_VALUE);

	onion_dict_add(B, "C", C, OD_DICT);

	onion_dict_add(C, "a", "hello", 0);

	FAIL_IF_NOT_EQUAL(onion_dict_rget(A, "B", NULL), NULL);
	FAIL_IF_NOT_EQUAL(onion_dict_rget(A, "C", NULL), NULL);
	FAIL_IF_NOT_EQUAL(onion_dict_rget(A, "B", "C", NULL), NULL);

	FAIL_IF_NOT_EQUAL(onion_dict_rget_dict(A, "B", NULL), B);
	FAIL_IF_NOT_EQUAL(onion_dict_rget_dict(A, "C", NULL), C);
	FAIL_IF_NOT_EQUAL(onion_dict_rget_dict(A, "B", "C", NULL), C);

	FAIL_IF_NOT_EQUAL_STR(onion_dict_rget(A, "B", "C", "a", NULL), "hello");
	FAIL_IF_NOT_EQUAL(onion_dict_rget_dict(A, "B", "C", "a", NULL), NULL);

	// This should remove all the others, as they hang from it.
	onion_dict_free(A);

	END_LOCAL();
}

void t14_dict_case_insensitive(){
	INIT_LOCAL();

	onion_dict *d=onion_dict_new();

	onion_dict_add(d,"Test","OK", 0);
	FAIL_IF_NOT_EQUAL(onion_dict_get(d,"test"),NULL);

	onion_dict_set_flags(d,OD_ICASE);
	FAIL_IF_NOT_EQUAL_STR(onion_dict_get(d,"test"),"OK");


	onion_dict_free(d);
	END_LOCAL();
}

void t15_hard_dup_dict_in_dict(){
	INIT_LOCAL();

	onion_dict *orig=onion_dict_new();

	char tmp[9];
	int i;
	for (i=0;i<256;i++){
		sprintf(tmp,"%08X",rand());
		onion_dict_add(orig, tmp, tmp, OD_DUP_ALL);
	}
	onion_dict_add(orig, "0", "no frees", 0);

	onion_dict *subdict=onion_dict_new();
	onion_dict_add(subdict,"subdict","test",0);
	onion_dict_add(orig, "subdict", subdict, OD_DICT|OD_FREE_VALUE);

	onion_dict *dest=onion_dict_hard_dup(orig);

	FAIL_IF(orig==dest);
	/// Check they have exactly the same keys.
	onion_dict_preorder(orig, cmpdict, dest);
	onion_dict_preorder(dest, cmpdict, orig);

	onion_dict_free(orig);
	onion_dict_free(dest);

	END_LOCAL();
}

void t16_soft_dup_dict_in_dict(){
	INIT_LOCAL();

	onion_dict *orig=onion_dict_new();

	char tmp[9];
	int i;
	for (i=0;i<256;i++){
		sprintf(tmp,"%08X",rand());
		onion_dict_add(orig, tmp, tmp, OD_DUP_ALL);
	}
	onion_dict_add(orig, "0", "no frees", 0);

	onion_dict *subdict=onion_dict_new();
	onion_dict_add(subdict,"subdict","test",0);
	onion_dict_add(orig, "subdict", subdict, OD_DICT|OD_FREE_VALUE);

	onion_dict *dest=onion_dict_dup(orig);

	FAIL_IF_NOT(orig==dest);

	/// Check they have exactly the same keys.
	onion_dict_preorder(orig, cmpdict, dest);
	onion_dict_preorder(dest, cmpdict, orig);

	onion_dict_free(orig);
	onion_dict_free(dest);

	END_LOCAL();
}

void t17_merge(){
	INIT_LOCAL();

	onion_dict *a=onion_dict_from_json("{\"hello\":\"world\"}");
	onion_dict *b=onion_dict_from_json("{\"bye\":\"_world_\", \"sub\": { \"hello\": \"world!\" } }");

	onion_dict_merge(a,b);

	FAIL_IF_NOT_EQUAL_STR(onion_dict_get(a,"bye"), "_world_");
	FAIL_IF_NOT_EQUAL_STR(onion_dict_rget(a,"sub","hello",NULL), "world!");

	onion_dict_free(b);
	FAIL_IF_NOT_EQUAL_STR(onion_dict_rget(a,"sub","hello",NULL), "world!");
	onion_dict_free(a);

	END_LOCAL();
}

void t18_json_escape_codes(){
	INIT_LOCAL();

	onion_dict *d=onion_dict_from_json("{ \"hello\": \"Hello\\nworld\", \"second\":\"second\" }");
	FAIL_IF_NOT_STRSTR(onion_dict_get(d, "hello"), "Hello\nworld");
	FAIL_IF_NOT_STRSTR(onion_dict_get(d, "second"), "second");
	onion_dict_free(d);

	d=onion_dict_from_json("{ \"hello\": \"\\uD83D\\uDE02\" }");
	FAIL_IF_NOT_STRSTR(onion_dict_get(d, "hello"), "😂");
	onion_dict_free(d);

	d=onion_dict_from_json("{ \"hello\": \"\\uD83D\\uDE03\" }"); // Another code point
	FAIL_IF_STRSTR(onion_dict_get(d, "hello"), "😂");
	onion_dict_free(d);

	d=onion_dict_from_json("{ \"hello\": \"\\u007b\" }"); // simple unicode
	FAIL_IF_NOT_STRSTR(onion_dict_get(d, "hello"), "{");
	onion_dict_free(d);

	d=onion_dict_from_json("{ \"hello\": \"\\\"Quote\" }"); // Escape quote
	FAIL_IF_NOT_STRSTR(onion_dict_get(d, "hello"), "\"Quote");
	onion_dict_free(d);

	d=onion_dict_from_json("{ \"hello\": \"\"Quote\" }"); // Must fail
	FAIL_IF_NOT_EQUAL(d, NULL);

	d=onion_dict_new();
	onion_dict_add(d, "hello", "Hello\nWorld\\", 0);
	onion_dict_add(d, "second", "123", 0);
	onion_block *b=onion_dict_to_json(d);
	FAIL_IF_NOT_EQUAL_STR(onion_block_data(b),"{\"hello\":\"Hello\\nWorld\\\\\", \"second\":\"123\"}");
	onion_block_free(b);
	onion_dict_free(d);

	d=onion_dict_new();
	onion_dict_add(d, "hello", "😂\t\n😂", 0);
	b=onion_dict_to_json(d);
	FAIL_IF_NOT_EQUAL_STR(onion_block_data(b),"{\"hello\":\"😂\\t\\n😂\"}");
	onion_block_free(b);
	onion_dict_free(d);

	d=onion_dict_new();
	onion_dict_add(d, "hello", "\02\03\x7f", 0);
	b=onion_dict_to_json(d);
	FAIL_IF_NOT_EQUAL_STR(onion_block_data(b),"{\"hello\":\"\\u0002\\u0003\\u007F\"}");
	onion_block_free(b);
	onion_dict_free(d);

	END_LOCAL();
}


int main(int argc, char **argv){
  START();
	t01_create_add_free();
	t01_create_add_free_10();
	t02_create_and_free_a_lot(100);
	t03_create_and_free_a_lot_random(100);
	t04_create_and_free_a_dup();
	t05_preorder();
	t06_null_add();
	t07_replace();
#ifdef HAVE_PTHREADS
	t08_threaded_lock();
	t09_thread_war();
#endif
	t10_tojson();
	t11_hard_dup();
	t12_dict_in_dict();
	t13_dict_rget();
  t14_dict_case_insensitive();
	t15_hard_dup_dict_in_dict();
	t16_soft_dup_dict_in_dict();
	t17_merge();
	t18_json_escape_codes();

	END();
}
