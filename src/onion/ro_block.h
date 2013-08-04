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

#ifndef __ONION_RO_BLOCK__
#define __ONION_RO_BLOCK__

#include "types.h"
#include <stdlib.h>
#include <ctype.h>

struct onion_ro_block_t{
	char *end;
	char *p;
};

static inline void onion_ro_block_init(onion_ro_block *bl, char *data, size_t size){
	bl->end=data+size;
	bl->p=data;
}

static inline char onion_ro_block_get_char(onion_ro_block *bl){
	return *bl->p++;
}
static inline char *onion_ro_block_get(onion_ro_block *bl){
	return bl->p;
}
static inline int onion_ro_block_eof(onion_ro_block *bl){
	return bl->p>=bl->end;
}
static inline size_t onion_ro_block_remaining(onion_ro_block *bl){
	return bl->end-bl->p;
}

char *onion_ro_block_get_token(onion_ro_block *bl, char delimiter);
char *onion_ro_block_get_to_nl(onion_ro_block *bl);
char *onion_ro_block_get_token2(onion_ro_block *bl, char *delimiter, char *rc);

char *onion_str_get_token(char **str, char delimiter);
char *onion_str_get_token2(char **str, char *delimiter, char *rc);
char *onion_str_strip(char *str);

#endif
