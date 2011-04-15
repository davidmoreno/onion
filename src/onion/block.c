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

#include "types_internal.h"
#include "block.h"
#include "log.h"

#include <malloc.h>
#include <string.h>

#define ONION_BLOCK_GROW_MIN_BLOCK 16
#define ONION_BLOCK_GROW_EXPONENTIAL_LIMIT 1024

/**
 * @short Creates a new block
 * 
 */
onion_block *onion_block_new(){
	onion_block *ret=malloc(sizeof(block));
	ret->data=malloc(ONION_BLOCK_GROW_MIN_BLOCK);
	ret->maxsize=ONION_BLOCK_GROW_MIN_BLOCK;
	ret->size=0;
	return ret;
}

/**
 * @short Removes the current block
 */
void onion_block_free(onion_block *bl){
	free(bl->data);
	free(bl);
}

/**
 * @short Discards all data on this block, and set the size to 0
 * 
 * This is usefull to reuse existing blocks
 */
void onion_block_clear(onion_block *b){
	b->size=0;
}

/**
 * @short Ensures the block has at least this reserved memory space.
 * 
 * This is usefull for some speedups, and prevent sucessive mallocs 
 * if you know beforehand the size.
 */
void onion_block_min_maxsize(onion_block *b, int minsize){
	if (b->maxsize < minsize){
		b->data=realloc(b->data, minsize);
		b->maxsize=minsize;
	}
}

/**
 * @short Returns the current data. 
 * 
 * It will be finished with a \0 (if not already, to ensure is printable.
 */
const char *onion_block_data(onion_block *b){
	if (b->size==b->maxsize){
		onion_block_add_char(b, 0);
		b->size--;
	}
	else // Its within limits
		 b->data[b->size]='\0';
	return b->data;
}

/**
 * @short Returns current block size
 */
off_t onion_block_size(onion_block *b){
	return b->size;
}

/**
 * @short Adds a character to the block
 */
void onion_block_add_char(onion_block *bl, char c){
	if (bl->size>=bl->maxsize){ // Grows ^2, until 1024, and then on 1024 chunks.
		int grow=bl->maxsize;
		if (grow>ONION_BLOCK_GROW_EXPONENTIAL_LIMIT)
			grow=ONION_BLOCK_GROW_EXPONENTIAL_LIMIT;
		bl->maxsize+=grow;
		bl->data=realloc(bl->data, bl->maxsize);
	}
	bl->data[bl->size++]=c;
}

/**
 * @short Adds a string to the block
 */
void onion_block_add_str(onion_block *b, const char *str){
	int l=strlen(str)+1;
	onion_block_add_data(b, str, l);
	b->size--;
}

/**
 * @short Adds raw data to the block
 */
void onion_block_add_data(onion_block *bl, const char *data, size_t l){
	// I have to perform manual realloc as if I append same block, realloc may free the data, so I do it manually.
	char *manualrealloc=NULL;
	if (bl->size+l>bl->maxsize){
		int grow=l;
		if (grow<ONION_BLOCK_GROW_MIN_BLOCK)
			grow=ONION_BLOCK_GROW_MIN_BLOCK;
		bl->maxsize=bl->size+grow;
		manualrealloc=bl->data;
		bl->data=malloc(bl->maxsize);
		memcpy(bl->data, manualrealloc, bl->size);
	}
	//ONION_DEBUG("Copy %d bytes, start at %d, max size is %d", l, bl->size, bl->maxsize);
	memmove(&bl->data[bl->size], data, l);
	bl->size+=l;
	if (manualrealloc)
		free(manualrealloc);
}

/**
 * @short Appends both blocks on the first.
 */
void onion_block_add_block(onion_block *b, onion_block *toadd){
	onion_block_add_data(b, toadd->data, toadd->size);
}

