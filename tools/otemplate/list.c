/*
	Onion HTTP server library
	Copyright (C) 2010-2011 David Moreno Montero

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
#include <stdio.h>

#include <onion/log.h>

#include "list.h"

list *list_new(void *free_function){
	list *l=malloc(sizeof(list));
	l->head=NULL;
	l->tail=NULL;
	l->free=free_function;
	return l;
}

void list_free(list *l){
	list_item *i=l->head;
	void (*f)(void *p);
	f=l->free;
	while (i){
		if (f && (i->flags & LIST_ITEM_FREE))
			f(i->data);
		list_item *last_i=i;
		i=i->next;
		free(last_i);
	}
	free(l);
}

void list_add(list *l, void *p){
	list_item *item=malloc(sizeof(list_item));
	item->data=p;
	item->next=NULL;
	item->flags = LIST_ITEM_FREE;
	item->prev=l->tail;
	if (l->tail)
		l->tail->next=item;
	l->tail=item;
	if (!l->head)
		l->head=item;
}

void list_add_with_flags(list *l, void *p, int flags) {
	list_item *item = malloc(sizeof(list_item));
	item->data = p;
	item->next=NULL;
	item->flags = flags;
	item->prev = l->tail;
	if(l->tail)
		l->tail->next=item;
	l->tail = item;
	if(!l->head)
		l->head=item;
}

void list_loop(list *l, void *ff, void *extra){
	void (*f)(void *extra, void *p);
	f=ff;
	list_item *i=l->head;
	while (i){
		f(extra, i->data);
		i=i->next;
	}
}

void list_pop(list *l){
	if (!l->tail)
		return;
	
	void (*f)(void *p);
	list_item *tail=l->tail;
	f=l->free;
	if (f)
		f(tail->data);
	
	l->tail=tail->prev;
	l->tail->next=NULL;
	free(tail);
}

int list_count(list *l){
	list_item *it=l->head;
	int c=0;
	while(it){
		c++;
		it=it->next;
	}
	return c;
}

void *list_get_n(list *l, int n){
	int i=0;
	list_item *it=l->head;
	while (it){
		if (i==n){
			//ONION_DEBUG("Found");
			return it->data;
		}
		i++;
		it=it->next;
	}
	return NULL;
}
