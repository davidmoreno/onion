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

#ifndef __BLOCK_H__
#define __BLOCK_H__

typedef struct block_t{
	char *data;
	int length;
	int pos;
}block;

block *block_new();
void block_free(block *);
void block_add_char(block *, char c);
void block_add_string(block *, const char *str);
void block_safe_for_printf(block *bl);

#endif
