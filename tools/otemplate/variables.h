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

#ifndef __VARIABLES_H__
#define __VARIABLES_H__

#include "parser.h"
#include <onion/types.h>

typedef enum{
	STRING=0,
	LITERAL=1,
	DICT=2
} vartype_e;

void variable_write(struct parser_status_t *st, onion_block *b);
void variable_solve(struct parser_status_t *st, const char *b, const char *tmpname, vartype_e type);

list *split(const char *str,char delimiter);

#endif
