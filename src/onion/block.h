/*
	Onion HTTP server library
	Copyright (C) 2011 David Moreno Montero

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 3.0 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not see <http://www.gnu.org/licenses/>.
	*/

#ifndef __BLOCK_H__
#define __BLOCK_H__

#ifdef __cplusplus
extern "C"{
#endif

#include "types.h"
#include <stddef.h>
#include <unistd.h>

onion_block *onion_block_new();
void onion_block_free(onion_block *b);
void onion_block_clear(onion_block *b);

void onion_block_min_maxsize(onion_block *b, int minsize);
off_t onion_block_size(const onion_block *b);
const char *onion_block_data(const onion_block *b);

void onion_block_rewind(onion_block *b, off_t n);

int onion_block_add_char(onion_block *b, char c);
int onion_block_add_str(onion_block *b, const char *str);
int onion_block_add_data(onion_block *b, const char *data, size_t length);
int onion_block_add_block(onion_block *b, onion_block *toadd);

#ifdef __cplusplus
}
#endif

#endif
