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

#include "onion_dict.h"

/**
 * Initializes the basic tree with all the structure in place, but empty.
 */
onion_dict *onion_dict_new(){
	onion_dict *dict=malloc(sizeof(onion_dict));
	memset(dict,0,sizeof(onion_dict));
	dict->flags=OD_EMPTY;
	return dict;
}

/**
 * @short Searchs for a given key, and returns that node and its parent (if parent!=NULL) 
 *
 * If not found, returns the parent where it should be. Nice for adding too.
 */
static onion_dict *onion_dict_find_node(onion_dict *dict, const char *key, onion_dict **parent){
	if (!dict || dict->flags&OD_EMPTY){
		return NULL;
	}
	char cmp=strcmp(key, dict->key);
	//fprintf(stderr,"Find %s, here %s, left %s, right %s, cmp %d\n",key,dict->key, dict->left ? dict->left->key : NULL, dict->right ? dict->right->key : NULL, cmp);
	if (cmp==0)
		return dict;
	if (parent) *parent=dict;
	if (cmp<0)
		return onion_dict_find_node(dict->left, key, parent);
	if (cmp>0)
		return onion_dict_find_node(dict->right, key, parent);
	return NULL;
}

/**
 * Adds a value in the tree.
 */
void onion_dict_add(onion_dict *dict, const char *key, const char *value, int flags){
	onion_dict *dup, *where=NULL;
	//fprintf(stderr,"Add %s\n",key);
	
	dup=onion_dict_find_node(dict, key, &where);
	
	if (dup){ // If dup, try again on left or right tree, it does not matter.
		if (!dup->left)
			dup->left=onion_dict_new();
		onion_dict_add(dup->left, key, value, flags);
		return;
	}
	if (where==NULL)
		where=dict;
	
	if (!(where->flags&OD_EMPTY)){
		dict=onion_dict_new();
		if (strcmp(key, where->key)<0)
			where->left=dict;
		else
			where->right=dict;
		where=dict;
	}
	
	if (flags&OD_DUP_KEY)
		where->key=strdup(key);
	else
		where->key=key;
	if (flags&OD_DUP_KEY)
		where->value=strdup(value);
	else
		where->value=value;
	where->flags=flags&~OD_EMPTY;
}

/// Frees the memory, if necesary of key and value
static void onion_dict_free_node_kv(onion_dict *dict){
	if (dict->flags&OD_DUP_KEY)
		free((char*)dict->key);
	if (dict->flags&OD_DUP_VALUE)
		free((char*)dict->value);
}

/// Copies internal data straight into dst. No free's nor any check.
static void onion_dict_copy_data(onion_dict *src, onion_dict *dst){
	dst->flags=src->flags;
	dst->key=src->key;
	dst->value=src->value;
	
	dst->right=src->right;
	dst->left=src->left;
}


/**
 * Removes the given key. 
 *
 * Returns if it removed any node.
 */ 
int onion_dict_remove(onion_dict *dict, const char *key){
	onion_dict *parent=NULL;
	dict=onion_dict_find_node(dict, key, &parent);

	if (!dict)
		return 0;
	onion_dict_free_node_kv(dict);
	if (dict->left && dict->right){ // I copy right here, and move left tree to leftmost branch.
		onion_dict *left=dict->left;
		onion_dict *t=dict->right;
		onion_dict_copy_data(dict->right, dict);
		
		while (t->left) t=t->left;
		
		t->left=left;
	}
	else if (dict->left){
		onion_dict_copy_data(dict->left, dict);
	}
	else if (dict->right){
		onion_dict_copy_data(dict->right, dict);
	}
	else{
		dict->flags=OD_EMPTY;
		dict->key=dict->value=NULL;
	}
	return 1;
}

/// Removes the full dict struct form mem.
void onion_dict_free(onion_dict *dict){
	if (dict->left)
		onion_dict_free(dict->left);
	if (dict->right)
		onion_dict_free(dict->right);

	onion_dict_free_node_kv(dict);
}

/// Gets a value
const char *onion_dict_get(onion_dict *dict, const char *key){
	onion_dict *r;
	//fprintf(stderr,"Get %s\n",key);
	r=onion_dict_find_node(dict, key, NULL);
	if (r)
		return r->value;
	return NULL;
}

