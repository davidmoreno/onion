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

#include "types_internal.h"
#include "low.h"
#include "ptr_list.h"

/**
 * @short Creates a new onion_ptr_list
 */
onion_ptr_list* onion_ptr_list_new()
{
	return NULL;
}

/**
 * @short Adds a ptr to the list. Elements are added to the head, so must use return value as new list pointer.
 */
onion_ptr_list* onion_ptr_list_add(onion_ptr_list* l, void* ptr)
{
	onion_ptr_list *n=onion_low_malloc(sizeof(onion_ptr_list));
	n->next=l;
	n->ptr=ptr;
	return n;
}

/**
 * @short Free the list, but do nothing on the ptrs.
 */
void onion_ptr_list_free(onion_ptr_list* l)
{
	if (l==NULL)
		return;
	onion_ptr_list *next=l->next;
	onion_low_free(l);
	onion_ptr_list_free(next); // Tail recursion, same as loop.
}

/**
 * @short Executes a function on all ptrs.
 * 
 * Internally is allowed to do this manually.
 */
void onion_ptr_list_foreach(onion_ptr_list* l, void (*f)(void *)){
	while(l){
		f(l->ptr);
		l=l->next;
	}
}
