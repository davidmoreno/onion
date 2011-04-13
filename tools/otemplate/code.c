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

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>

#include "onion/log.h"
#include "list.h"
#include "block.h"
#include "parser.h"

typedef enum token_type_e{
	T_VAR=1,
	T_STRING=2,
}token_type;

typedef struct code_token_t{
	char *data;
	token_type type;
}code_token;

code_token *code_token_new(char *data, int l, token_type t){
	code_token *ret=malloc(sizeof(code_token));
	ret->data=strndup(data,l);
	ret->type=t;
	return ret;
}

void code_token_free(code_token *t){
	free(t->data);
	free(t);
}

void code_load(parser_status *st, list *l);
void code_for(parser_status *st, list *l);
void code_endfor(parser_status *st, list *l);
void code_if(parser_status *st, list *l);
void code_endif(parser_status *st, list *l);
void code_trans(parser_status *st, list *l);

void write_code(parser_status *st, block *b){
	list *command=list_new((void*)code_token_free);
	
	char mode=0; // 0 skip spaces, 1 in single var, 2 in quotes
	
	int i, li=0;
	for (i=0;i<b->pos;i++){
		char c=b->data[i];
		switch(mode){
			case 0:
				if (!isspace(c)){
					if (c=='"'){
						mode=2;
						li=i+1;
					}
					else{
						mode=1;
						li=i;
					}
				}
				break;
			case 1:
				if (isspace(c)){
					mode=0;
					list_add(command, code_token_new(&b->data[li], i-li, T_VAR));
				}
				break;
			case 2:
				if (c=='"'){
					mode=0;
					list_add(command, code_token_new(&b->data[li], i-li, T_STRING));
				}
				break;
		}
	}
	
	if (!command->head){
		ONION_ERROR("Incomplete command");
		st->status=1;
		return;
	}
	
	code_token *commandtoken=command->head->data;
	const char *commandname=commandtoken->data;
	
	if (strcmp(commandname,"load")==0)
		code_load(st, command);
	else if (strcmp(commandname,"for")==0)
		code_for(st, command);
	else if (strcmp(commandname,"endfor")==0)
		code_endfor(st, command);
	else if (strcmp(commandname,"if")==0)
		code_for(st, command);
	else if (strcmp(commandname,"endif")==0)
		code_endfor(st, command);
	else if (strcmp(commandname,"trans")==0)
		code_trans(st, command);
	else{
		ONION_ERROR("Unknown command '%s'. Ignoring.", commandname);
		st->status=1;
	}
}


void code_load(parser_status *st, list *l){
	list_item *it=l->head->next;
	while (it){
		const char *modulename=((code_token*)it->data)->data;

		ONION_INFO("Loading external module %s",modulename);
		
		it=it->next;
	}
}
void code_for(parser_status *st, list *l){
	ONION_ERROR("Not yet");
	st->status=1;
}
void code_endfor(parser_status *st, list *l){
	ONION_ERROR("Not yet");
	st->status=1;
}
void code_if(parser_status *st, list *l){
	ONION_ERROR("Not yet");
	st->status=1;
}
void code_endif(parser_status *st, list *l){
	ONION_ERROR("Not yet");
	st->status=1;
}

void code_trans(parser_status *st, list *l){
	block *tmp=block_new(NULL);
	block_add_string(tmp, ((code_token*)l->head->next->data)->data);
	block_safe_for_printf(tmp);
	fprintf(st->out, "  onion_response_write0(res, gettext(\"%s\"));\n", tmp->data);
	block_free(tmp);
}
