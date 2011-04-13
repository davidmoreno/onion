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

#include "block.h"
#include <malloc.h>
#include <ctype.h>
#include <string.h>

block *block_new(void *extra){
	block *ret=malloc(sizeof(block));
	ret->data=malloc(16);
	ret->length=16;
	ret->pos=0;
	ret->extra=extra;
	return ret;
}

void block_free(block *bl){
	free(bl->data);
	free(bl);
}

void block_add_char(block *bl, char c){
	if (bl->pos>=bl->length){
		bl->data=realloc(bl->data, bl->length*2);
		bl->length*=2;
	}
	bl->data[bl->pos++]=c;
}

void block_add_string(block *bl, const char *str){
	int l=strlen(str)+1;
	if (bl->pos+l>bl->length){
		bl->data=realloc(bl->data, bl->pos+l);
		bl->length=bl->pos+l;
	}
	memcpy(&bl->data[bl->pos], str, l);
	bl->pos+=l-1;
}


void block_safe_for_printf(block *bl){
	block *tmp=block_new(NULL);
	
	int i;
	for (i=0;i<bl->pos;i++){
		char c=bl->data[i];
		if ((isspace(c) || isgraph(c)) && c!='\n' && c!='"')
			block_add_char(tmp, c);
		else{
			char tmpc[8];
			snprintf(tmpc, sizeof(tmpc), "%03o", c);
			block_add_char(tmp, '\\');
			block_add_char(tmp, tmpc[0]);
			block_add_char(tmp, tmpc[1]);
			block_add_char(tmp, tmpc[2]);
		}
	}
	block_add_char(tmp, '\0');
	free(bl->data);
	bl->data=tmp->data;
	bl->length=tmp->length;
	bl->pos=tmp->pos;
	free(tmp);
}
