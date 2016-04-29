/*
	Onion HTTP server library
	Copyright (C) 2010-2016 David Moreno Montero and others

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
#include "block.h"
#include "log.h"
#include "low.h"

#include <stdlib.h>
#include <string.h>

#define ONION_BLOCK_GROW_MIN_BLOCK 16
#define ONION_BLOCK_GROW_EXPONENTIAL_LIMIT 1024

/// @defgroup block Block. Variable size bytes blocks. Used for some internal string representation.

/**
 * @short Creates a new block
 * @memberof onion_block_t
 * @ingroup block
 */
onion_block *onion_block_new(){
	onion_block *ret=onion_low_malloc(sizeof(onion_block));
	ret->data=onion_low_scalar_malloc(ONION_BLOCK_GROW_MIN_BLOCK);
	ret->maxsize=ONION_BLOCK_GROW_MIN_BLOCK;
	ret->size=0;
	return ret;
}

/**
 * @short Removes the current block
 * @memberof onion_block_t
 * @ingroup block
 */
void onion_block_free(onion_block *bl){
	onion_low_free(bl->data);
	onion_low_free(bl);
}

/**
 * @short Discards all data on this block, and set the size to 0
 * @memberof onion_block_t
 * @ingroup block
 *
 * This is usefull to reuse existing blocks
 */
void onion_block_clear(onion_block *b){
	b->size=0;
}

/**
 * @short Ensures the block has at least this reserved memory space.
 * @memberof onion_block_t
 * @ingroup block
 *
 * This is usefull for some speedups, and prevent sucessive mallocs
 * if you know beforehand the size.
 */
void onion_block_min_maxsize(onion_block *b, int minsize){
	if (b->maxsize < minsize){
		b->data=onion_low_realloc(b->data, minsize);
		b->maxsize=minsize;
	}
}

/**
 * @short Returns the current data.
 * @memberof onion_block_t
 * @ingroup block
 *
 * It will be finished with a \0 (if not already, to ensure is printable.
 */
const char *onion_block_data(const onion_block *b){
	// It can really modify the size, as it ensures a \0 at the end, but its ok
	if (b->size==b->maxsize){
		onion_block_add_char((onion_block*)b, 0);
		((onion_block*)b)->size--;
	}
	else // Its within limits
		 b->data[b->size]='\0';
	return b->data;
}

/**
 * @short Returns current block size
 * @memberof onion_block_t
 * @ingroup block
 */
off_t onion_block_size(const onion_block *b){
	return b->size;
}

/**
 * @short Reduces the size of the block.
 * @memberof onion_block_t
 * @ingroup block
 */
void onion_block_rewind(onion_block *b, off_t n){
	if (b->size<n)
		b->size=0;
	else
		b->size-=n;
}


/**
 * @short Adds a character to the block
 * @memberof onion_block_t
 */
int onion_block_add_char(onion_block *bl, char c){
	if (bl->size>=bl->maxsize){ // Grows ^2, until 1024, and then on 1024 chunks.
		int grow=bl->maxsize;
		if (grow>ONION_BLOCK_GROW_EXPONENTIAL_LIMIT)
			grow=ONION_BLOCK_GROW_EXPONENTIAL_LIMIT;
		bl->maxsize+=grow;
		bl->data=onion_low_realloc(bl->data, bl->maxsize);
	}
	bl->data[bl->size++]=c;
	return 1;
}

/**
 * @short Adds a string to the block
 * @memberof onion_block_t
 */
int onion_block_add_str(onion_block *b, const char *str){
	int l=strlen(str)+1;
	onion_block_add_data(b, str, l);
	b->size--;
	return l;
}

/**
 * @short Adds raw data to the block
 * @memberof onion_block_t
 */
int onion_block_add_data(onion_block *bl, const char *data, size_t l){
	// I have to perform manual realloc as if I append same block, realloc may free the data, so I do it manually.
	char *manualrealloc=NULL;
	if (bl->size+l>bl->maxsize){
		int grow=l;
		if (grow<ONION_BLOCK_GROW_MIN_BLOCK)
			grow=ONION_BLOCK_GROW_MIN_BLOCK;
		bl->maxsize=bl->size+grow;
		manualrealloc=bl->data;
		bl->data=onion_low_scalar_malloc(bl->maxsize);
		memcpy(bl->data, manualrealloc, bl->size);
	}
	//ONION_DEBUG("Copy %d bytes, start at %d, max size is %d", l, bl->size, bl->maxsize);
	memmove(&bl->data[bl->size], data, l);
	bl->size+=l;
	if (manualrealloc)
		onion_low_free(manualrealloc);
	return l;
}

/**
 * @short Appends both blocks on the first.
 * @memberof onion_block_t
 */
int onion_block_add_block(onion_block *b, onion_block *toadd){
	return onion_block_add_data(b, toadd->data, toadd->size);
}
