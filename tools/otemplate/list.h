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

#ifndef __LIST_H__
#define __LIST_H__

enum LIST_FREE_FLAGS {
	LIST_ITEM_NO_FREE,
	LIST_ITEM_FREE
};

typedef struct list_item_t{
	void *data;
	struct list_item_t *next;
	struct list_item_t *prev;
	int flags;
}list_item;

typedef struct list_t{
	list_item *head;
	list_item *tail;
	
	void *free;
}list;

list *list_new(void *free_function);
void list_free(list *l);
void list_add(list *l, void *p);
void list_add_with_flags(list *l, void *p, int flags);
void list_loop(list *l, void *f, void *extra);
void list_pop(list *l);
int list_count(list *l);
void *list_get_n(list *l, int n);

#endif
