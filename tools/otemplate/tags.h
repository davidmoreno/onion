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

#ifndef __TAGS_H__
#define __TAGS_H__

#include "parser.h"

#include <onion/types.h>

/// Types of tokens.
typedef enum token_type_e {
  T_VAR = 0,
  T_STRING = 1,
} token_type;

/// Pack of token and type. At the list passed to the tag handlers each element is one of these.
typedef struct tag_token_t {
  char *data;
  token_type type;
} tag_token;

void tag_write(parser_status * st, onion_block * b);
void tag_add(const char *tag, void *f);
void tag_free();
void tag_init();

const char *tag_value_arg(list * l, int n);
int tag_type_arg(list * l, int n);

#endif
