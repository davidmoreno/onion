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

#ifndef __ONION_RO_BLOCK__
#define __ONION_RO_BLOCK__

#include "types.h"
#include <stdlib.h>
#include <ctype.h>

struct onion_parser_block_t{
	char *end;
	char *p;
};

static inline void onion_parser_block_init(onion_parser_block *bl, char *data, size_t size){
	bl->end=data+size;
	bl->p=data;
}

static inline char onion_parser_block_get_char(onion_parser_block *bl){
	return *bl->p++;
}
static inline char *onion_parser_block_get(onion_parser_block *bl){
	return bl->p;
}
static inline int onion_parser_block_eof(onion_parser_block *bl){
	return bl->p>=bl->end;
}
static inline size_t onion_parser_block_remaining(onion_parser_block *bl){
	return bl->end-bl->p;
}
static inline void onion_parser_block_advance(onion_parser_block *bl, ssize_t l){
	bl->p+=l;
}

char *onion_parser_block_get_token(onion_parser_block *bl, char delimiter);
char *onion_parser_block_get_to_nl(onion_parser_block *bl);
char *onion_parser_block_get_token2(onion_parser_block *bl, char *delimiter, char *rc);

char *onion_str_get_token(char **str, char delimiter);
char *onion_str_get_token2(char **str, char *delimiter, char *rc);
char *onion_str_strip(char *str);
char *onion_str_unquote(char *str);

#endif
