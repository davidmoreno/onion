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

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>

#include "onion/log.h"
#include "list.h"
#include "block.h"
#include "parser.h"
#include <libgen.h>

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
void code_else(parser_status *st, list *l);
void code_endif(parser_status *st, list *l);
void code_trans(parser_status *st, list *l);
void code_include(parser_status *st, list *l);

void write_code(parser_status *st, block *b){
	//ONION_DEBUG("Write code %s",b->data);
	
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
		code_if(st, command);
	else if (strcmp(commandname,"else")==0)
		code_else(st, command);
	else if (strcmp(commandname,"endif")==0)
		code_endif(st, command);
	else if (strcmp(commandname,"trans")==0)
		code_trans(st, command);
	else if (strcmp(commandname,"include")==0)
		code_include(st, command);
	else{
		ONION_ERROR("Unknown command '%s'. Ignoring.", commandname);
		st->status=1;
	}
	
	list_free(command);
}


void code_load(parser_status *st, list *l){
	list_item *it=l->head->next;
	while (it){
		const char *modulename=((code_token*)it->data)->data;

		ONION_WARNING("Loading external module %s not implemented yet.",modulename);
		
		it=it->next;
	}
}
void code_for(parser_status *st, list *l){
	parser_add_text(st, "  tmp=onion_dict_get(context, \"%s\");\n", ((code_token*)list_get_n(l,3))->data);
	parser_add_text(st, 
"  {\n"
"    onion_dict *tmpcontext=onion_dict_dup(context);\n"
"    while (*tmp){\n"
"      char tmp2[2];\n"
"      tmp2[0]=*tmp++; tmp2[1]='\\0';\n"
"      onion_dict_add(tmpcontext, \"%s\", tmp2, OD_DUP_VALUE|OD_REPLACE);\n", ((code_token*)list_get_n(l,1))->data
	);
	function_new(st);
}
void code_endfor(parser_status *st, list *l){
	function_data *d=parser_stack_pop(st);
	parser_add_text(st, "     %s(tmpcontext, res);\n",d->id);
	parser_add_text(st, "    }\n    onion_dict_free(tmpcontext);\n  }\n", d->id);
}

void code_if(parser_status *st, list *l){
	parser_add_text(st, "  tmp=onion_dict_get(context, \"%s\");\n", ((code_token*)list_get_n(l,1))->data);
	parser_add_text(st, "  if (!tmp || strcmp(tmp, \"false\")==0)\n");
	function_new(st);
}

void code_else(parser_status *st, list *l){
	function_data *d=parser_stack_pop(st);
	parser_add_text(st, "    %s(context, res);\n  else\n", d->id);
	
	function_new(st);
}

void code_endif(parser_status *st, list *l){
	function_data *d=parser_stack_pop(st);
	parser_add_text(st, "    %s(context, res);\n", d->id);
}

void code_trans(parser_status *st, list *l){
	block *tmp=block_new();
	block_add_string(tmp, ((code_token*)l->head->next->data)->data);
	block_safe_for_printf(tmp);
	parser_add_text(st, "  onion_response_write0(res, gettext(\"%s\"));\n", tmp->data);
	block_free(tmp);
}

void code_include(parser_status* st, list* l){
	function_new(st);
	
	FILE *oldin=st->in;
	char tmp[256];
	snprintf(tmp, sizeof(tmp), "%s/%s", dirname(strdupa(st->infilename)), ((code_token*)list_get_n(l,1))->data);
	ONION_DEBUG("Open file %s, relative to %s",tmp, st->infilename);
	st->in=fopen(tmp, "rt");
	const char *tmpinfilename=st->infilename;
	st->infilename=tmp;
	if (!st->in){
		ONION_ERROR("Could not open include file %s", ((code_token*)list_get_n(l,1))->data);
		st->status=1;
		st->in=oldin;
		st->infilename=tmpinfilename;
		parser_stack_pop(st);
		return;
	}
	st->rawblock->pos=0;
	st->last_wmode=0;
	int mode=st->mode;
	st->mode=TEXT;
	// Real job here, all around is to use this
	parse_template(st);
	
	fclose(st->in);
	st->in=oldin;
	st->infilename=tmpinfilename;
	st->mode=mode;
	
	function_data *d=parser_stack_pop(st);
	
	free(d->id);
	d->id=malloc(64);
	snprintf(d->id, 64, "ot_%s", basename(tmp));
	
	char *p=d->id;
	while (*p){
		if (*p=='.')
			*p='_';
		p++;
	}
	
	parser_add_text(st, "  %s(context, res);\n", d->id);
}
