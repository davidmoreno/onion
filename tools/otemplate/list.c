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

#include <malloc.h>
#include <stdio.h>

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
	item->prev=l->tail;
	if (l->tail)
		l->tail->next=item;
	l->tail=item;
	if (!l->head)
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
