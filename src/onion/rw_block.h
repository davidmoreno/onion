/*
	Onion HTTP server library
	Copyright (C) 2010-2013 David Moreno Montero

	This library is free software; you can redistribute it and/or
	modify it under the terms of, at your choice:
	
	a. the GNU Lesser General Public License as published by the 
	 Free Software Foundation; either version 3.0 of the License, 
	 or (at your option) any later version.
	
	b. the GNU General Public License as published by the 
	 Free Software Foundation; either version 2.0 of the License, 
	 or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License and the GNU General Public License along with this 
	library; if not see <http://www.gnu.org/licenses/>.
	*/

#ifndef __ONION_RW_BLOCK__
#define __ONION_RW_BLOCK__

#include "types.h"
#include <stdlib.h>

struct onion_rw_block_t{
	const char *start;
	const char *end;
	const char *p;
};

static inline void onion_rw_block_init(onion_rw_block *bl, const char *data, size_t size){
	bl->start=data;
	bl->end=data+size;
	bl->p=data;
}

static inline char onion_rw_block_get_char(onion_rw_block *bl){
	return *bl->p++;
}
static inline const char *onion_rw_block_get(onion_rw_block *bl){
	return bl->p;
}
static inline int onion_rw_block_eof(onion_rw_block *bl){
	return bl->p>=bl->end;
}
static inline size_t onion_rw_block_remaining(onion_rw_block *bl){
	return bl->end-bl->p;
}

#endif
