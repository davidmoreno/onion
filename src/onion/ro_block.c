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

#include "ro_block.h"

char *onion_ro_block_get_token(onion_ro_block *bl, char delimiter){
	if (onion_ro_block_eof(bl))
		return NULL;
	char *token=bl->p;
	while(*bl->p!=delimiter && !onion_ro_block_eof(bl)){
		bl->p++;
	}
	
	*bl->p=0;
	bl->p++;
	return token;
}
char *onion_ro_block_get_token2(onion_ro_block *bl, char *delimiter, char *rc){
	if (onion_ro_block_eof(bl))
		return NULL;
	char *token=bl->p;
	char c=*bl->p;
	while(c!=delimiter[0] && c!=delimiter[1] && !onion_ro_block_eof(bl)){
		bl->p++;
		c=*bl->p;
	}
	if (rc)
		*rc=c;
	*bl->p=0;
	bl->p++;
	return token;
}

char *onion_ro_block_get_to_nl(onion_ro_block *bl){
	char *r=onion_ro_block_get_token(bl, '\n');

	if (r && ( (bl->p-r) > 1) && bl->p[-2]=='\r') // remove \r if there is something that has data, and last char is \r.
		bl->p[-2]='\0';

	return r;
}

char *onion_str_get_token(char **str, char delimiter){
	char *s=*str;
	if (!*s)
		return NULL;
	char *token=s;
	while(*s!=delimiter && *s){
		s++;
	}
	if (*s){
		*s=0;
		*str=s+1;
	}
	else 
		*str=s;
	return token;
}
char *onion_str_get_token2(char **str, char *delimiter, char *rc){
	char *s=*str;
	if (!*s)
		return NULL;
	char *token=s;
	char c=*s;
	while(c!=delimiter[0] && c!=delimiter[1] && *s){
		s++;
		c=*s;
	}
	if (*s){
		*s=0;
		*str=s+1;
	}
	else 
		*str=s;
	return token;
}

char *onion_str_strip(char *s){
	while (*s && isspace(*s))
		s++;
	char *r=s;
	
	return r;
}
