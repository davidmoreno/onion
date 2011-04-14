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

typedef struct tag_token_t{
	char *data;
	token_type type;
}tag_token;

tag_token *tag_token_new(char *data, int l, token_type t){
	tag_token *ret=malloc(sizeof(tag_token));
	ret->data=strndup(data,l);
	ret->type=t;
	return ret;
}

void tag_token_free(tag_token *t){
	free(t->data);
	free(t);
}

void tag_load(parser_status *st, list *l);
void tag_for(parser_status *st, list *l);
void tag_endfor(parser_status *st, list *l);
void tag_if(parser_status *st, list *l);
void tag_else(parser_status *st, list *l);
void tag_endif(parser_status *st, list *l);
void tag_trans(parser_status *st, list *l);
void tag_include(parser_status *st, list *l);

void write_tag(parser_status *st, block *b){
	//ONION_DEBUG("Write tag %s",b->data);
	
	list *command=list_new((void*)tag_token_free);
	
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
					list_add(command, tag_token_new(&b->data[li], i-li, T_VAR));
				}
				break;
			case 2:
				if (c=='"'){
					mode=0;
					list_add(command, tag_token_new(&b->data[li], i-li, T_STRING));
				}
				break;
		}
	}
	
	if (!command->head){
		ONION_ERROR("Incomplete command");
		st->status=1;
		return;
	}
	
	tag_token *commandtoken=command->head->data;
	const char *commandname=commandtoken->data;
	
	if (strcmp(commandname,"load")==0)
		tag_load(st, command);
	else if (strcmp(commandname,"for")==0)
		tag_for(st, command);
	else if (strcmp(commandname,"endfor")==0)
		tag_endfor(st, command);
	else if (strcmp(commandname,"if")==0)
		tag_if(st, command);
	else if (strcmp(commandname,"else")==0)
		tag_else(st, command);
	else if (strcmp(commandname,"endif")==0)
		tag_endif(st, command);
	else if (strcmp(commandname,"trans")==0)
		tag_trans(st, command);
	else if (strcmp(commandname,"include")==0)
		tag_include(st, command);
	else{
		ONION_ERROR("Unknown command '%s'. Ignoring.", commandname);
		st->status=1;
	}
	
	list_free(command);
}


void tag_load(parser_status *st, list *l){
	list_item *it=l->head->next;
	while (it){
		const char *modulename=((tag_token*)it->data)->data;

		ONION_WARNING("Loading external module %s not implemented yet.",modulename);
		
		it=it->next;
	}
}
void tag_for(parser_status *st, list *l){
	template_add_text(st, "  tmp=onion_dict_get(context, \"%s\");\n", ((tag_token*)list_get_n(l,3))->data);
	template_add_text(st, 
"  {\n"
"    onion_dict *tmpcontext=onion_dict_dup(context);\n"
"    while (*tmp){\n"
"      char tmp2[2];\n"
"      tmp2[0]=*tmp++; tmp2[1]='\\0';\n"
"      onion_dict_add(tmpcontext, \"%s\", tmp2, OD_DUP_VALUE|OD_REPLACE);\n", ((tag_token*)list_get_n(l,1))->data
	);
	function_new(st);
}
void tag_endfor(parser_status *st, list *l){
	function_data *d=template_stack_pop(st);
	template_add_text(st, "     %s(tmpcontext, res);\n",d->id);
	template_add_text(st, "    }\n    onion_dict_free(tmpcontext);\n  }\n", d->id);
}

void tag_if(parser_status *st, list *l){
	template_add_text(st, "  tmp=onion_dict_get(context, \"%s\");\n", ((tag_token*)list_get_n(l,1))->data);
	template_add_text(st, "  if (!tmp || strcmp(tmp, \"false\")==0)\n");
	function_new(st);
}

void tag_else(parser_status *st, list *l){
	function_data *d=template_stack_pop(st);
	template_add_text(st, "    %s(context, res);\n  else\n", d->id);
	
	function_new(st);
}

void tag_endif(parser_status *st, list *l){
	function_data *d=template_stack_pop(st);
	template_add_text(st, "    %s(context, res);\n", d->id);
}

void tag_trans(parser_status *st, list *l){
	block *tmp=block_new();
	block_add_string(tmp, ((tag_token*)l->head->next->data)->data);
	block_safe_for_printf(tmp);
	template_add_text(st, "  onion_response_write0(res, gettext(\"%s\"));\n", tmp->data);
	block_free(tmp);
}

void tag_include(parser_status* st, list* l){
	function_new(st);
	
	FILE *oldin=st->in;
	char tmp[256];
	snprintf(tmp, sizeof(tmp), "%s/%s", dirname(strdupa(st->infilename)), ((tag_token*)list_get_n(l,1))->data);
	ONION_DEBUG("Open file %s, relative to %s",tmp, st->infilename);
	st->in=fopen(tmp, "rt");
	const char *tmpinfilename=st->infilename;
	st->infilename=tmp;
	if (!st->in){
		ONION_ERROR("Could not open include file %s", ((tag_token*)list_get_n(l,1))->data);
		st->status=1;
		st->in=oldin;
		st->infilename=tmpinfilename;
		template_stack_pop(st);
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
	
	function_data *d=template_stack_pop(st);
	
	free(d->id);
	d->id=malloc(64);
	snprintf(d->id, 64, "ot_%s", basename(tmp));
	
	char *p=d->id;
	while (*p){
		if (*p=='.')
			*p='_';
		p++;
	}
	
	template_add_text(st, "  %s(context, res);\n", d->id);
}
