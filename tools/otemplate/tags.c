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

/**
 * Current block is a tag, slice it and call the proper handler.
 */
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

/// Returns the nth arg oof the tag
const char *t_arg(list *l, int n){
	tag_token *t=list_get_n(l,n);
	if (t)
		return t->data;
	return NULL;
}

/// Loads an external handler set
void tag_load(parser_status *st, list *l){
	list_item *it=l->head->next;
	while (it){
		const char *modulename=((tag_token*)it->data)->data;

		ONION_WARNING("Loading external module %s not implemented yet.",modulename);
		
		it=it->next;
	}
}

/// Do the first for part.
void tag_for(parser_status *st, list *l){
	function_add_code(st, 
"  {\n"
"    onion_dict *loopdict=onion_dict_get_dict(context, \"%s\");\n", ((tag_token*)list_get_n(l,3))->data);
	function_add_code(st, 
"    onion_dict *tmpcontext=onion_dict_hard_dup(context);\n"
"    if (loopdict){\n"
"      dict_res dr={ .dict = tmpcontext, .res=res };\n"
"      onion_dict_preorder(loopdict, ");
	function_data *d=function_new(st, NULL);
	d->signature="dict_res *dr, const char *key, const void *value, int flags";

	function_add_code(st, 
"  onion_dict_add(dr->dict, \"%s\", value, OD_DUP_VALUE|OD_REPLACE);\n", ((tag_token*)list_get_n(l,1))->data);
	
	function_new(st, NULL);
}

/// Ends a for
void tag_endfor(parser_status *st, list *l){
	// First the preorder function
	function_data *d=function_pop(st);
	function_add_code(st, "  %s(dr->dict, dr->res);\n", d->id);
	
	// Now the normal code
	d=function_pop(st);
	function_add_code(st, "%s, &dr);\n"
"    }\n"
"    onion_dict_free(tmpcontext);\n"
"  }\n", d->id);
}

/// Starts an if
void tag_if(parser_status *st, list *l){
	function_add_code(st, 
"  {\n"
"    const char *tmp=onion_dict_get(context, \"%s\");\n"
"    if (!tmp || strcmp(tmp, \"false\")==0)\n", t_arg(l,1));
	function_new(st, NULL);
}

/// Else part
void tag_else(parser_status *st, list *l){
	function_data *d=function_pop(st);
	function_add_code(st, "    %s(context, res);\n  else\n", d->id);
	
	function_new(st, NULL);
}

/// endif
void tag_endif(parser_status *st, list *l){
	function_data *d=function_pop(st);
	function_add_code(st, "    %s(context, res);\n  }\n", d->id);
}

/// Following text is for gettext
void tag_trans(parser_status *st, list *l){
	block *tmp=block_new();
	block_add_string(tmp, ((tag_token*)l->head->next->data)->data);
	block_safe_for_printf(tmp);
	function_add_code(st, "  onion_response_write0(res, gettext(\"%s\"));\n", tmp->data);
	block_free(tmp);
}

/// Include an external html. This is only the call, the programmer must compile such html too.
void tag_include(parser_status* st, list* l){
	function_data *d=function_new(st, "%s", t_arg(l, 1));
	function_pop(st);
	block_free(d->code); // This means no impl
	d->code=NULL; 
	
	function_add_code(st, "  %s(context, res);\n", d->id);
}
