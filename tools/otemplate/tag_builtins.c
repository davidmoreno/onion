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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <assert.h>

#include <onion/log.h>
#include <onion/codecs.h>
#include <onion/block.h>

#include "functions.h"
#include "parser.h"
#include "tags.h"
#include "variables.h"
#include "load.h"

void tag_load(parser_status *st, list *l);
void tag_for(parser_status *st, list *l);
void tag_endfor(parser_status *st, list *l);
void tag_if(parser_status *st, list *l);
void tag_else(parser_status *st, list *l);
void tag_endif(parser_status *st, list *l);
void tag_include(parser_status *st, list *l);
void tag_extends(parser_status *st, list *l);
void tag_block(parser_status *st, list *l);
void tag_endblock(parser_status *st, list *l);

/**
 * @short Initializes all builtins
 */
void tag_init_builtins(){
	tag_add("load", tag_load);
	tag_add("for", tag_for);
	tag_add("endfor", tag_endfor);
	tag_add("if", tag_if);
	tag_add("else", tag_else);
	tag_add("endif", tag_endif);
	tag_add("include", tag_include);
	tag_add("extends", tag_extends);
	tag_add("block", tag_block);
	tag_add("endblock", tag_endblock);
}


/// Loads an external handler set
void tag_load(parser_status *st, list *l){
	list_item *it=l->head->next;
	while (it){
		const char *modulename=((tag_token*)it->data)->data;

		//ONION_WARNING("Loading external module %s not implemented yet.",modulename);
		
		if (load_external(modulename)!=0){
			ONION_ERROR("%s:%d here", st->infilename, st->line);
			st->status=1;
		}
		
		it=it->next;
	}
}

/// Do the first for part.
void tag_for(parser_status *st, list *l){
	function_add_code(st, 
"  {\n"
"    onion_dict *loopdict=NULL;\n"
	);
	variable_solve(st, tag_value_arg (l,3), "loopdict", 2);
// "    onion_dict_get_dict(context, \"%s\");\n", tag_value_arg (l,3));
	function_add_code(st, 
"    onion_dict *tmpcontext=onion_dict_hard_dup(context);\n"
"    if (loopdict){\n"
"      dict_res dr={ .dict = tmpcontext, .res=res };\n"
"      onion_dict_preorder(loopdict, ");
	function_data *d=function_new(st, NULL);
	d->signature="dict_res *dr, const char *key, const void *value, int flags";

	function_add_code(st, 
"  onion_dict_add(dr->dict, \"%s\", value, OD_DUP_VALUE|OD_REPLACE|(flags&OD_TYPE_MASK));\n", tag_value_arg(l,1));
	
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
	int lc=list_count(l);
	if (lc==2){
		function_add_code(st, 
"  {\n"
"    const char *tmp;\n"
		);
		variable_solve(st, tag_value_arg(l, 1), "tmp", tag_type_arg(l,1));
		function_add_code(st, 
"    if (tmp && strcmp(tmp, \"false\")!=0)\n", tag_value_arg(l,1));
	}
	else if (lc==4){
		const char *op=tag_value_arg(l, 2);
		const char *opcmp=NULL;
		if (strcmp(op,"==")==0)
			opcmp="==0";
		else if (strcmp(op,"<=")==0)
			opcmp="<=0";
		else if (strcmp(op,"<")==0)
			opcmp="<0";
		else if (strcmp(op,">=")==0)
			opcmp=">=0";
		else if (strcmp(op,">")==0)
			opcmp=">0";
		else if (strcmp(op,"!=")==0)
			opcmp="!=0";
		if (opcmp){
			function_add_code(st,
"  {\n"
"    const char *op1, *op2;\n");
			variable_solve(st, tag_value_arg(l, 1), "op1", tag_type_arg(l,1));
			variable_solve(st, tag_value_arg(l, 3), "op2", tag_type_arg(l,3));
			function_add_code(st,
"    if (op1==op2 || (op1 && op2 && strcmp(op1, op2)%s))\n",opcmp);
		}
		else{
			ONION_ERROR("%s:%d Unkonwn operator for if: %s", st->infilename, st->line, op);
			st->status=1;
		}
	}
	else{
		ONION_ERROR("%s:%d If only allows 1 or 3 arguments. TODO. Has %d.", st->infilename, st->line, lc-1);
		st->status=1;
	}
	function_new(st, NULL);
}

/// Else part
void tag_else(parser_status *st, list *l){
	function_data *d=function_pop(st);
	function_add_code(st, "      %s(context, res);\n    else\n", d->id);
	
	function_new(st, NULL);
}

/// endif
void tag_endif(parser_status *st, list *l){
	function_data *d=function_pop(st);
	function_add_code(st, "      %s(context, res);\n  }\n", d->id);
}

/// Include an external html. This is only the call, the programmer must compile such html too.
void tag_include(parser_status* st, list* l){
	assert(st!=NULL); // Tell coverty that at function_new it will keep a pointer to the original, always.
	
	function_data *d=function_new(st, "%s", tag_value_arg(l, 1));
	function_pop(st);
	onion_block_free(d->code); // This means no impl
	d->code=NULL; 
	
	function_add_code(st, "  %s(context, res);\n", d->id);
}


void tag_extends(parser_status *st, list *l){
	function_data *d=function_new(st, "%s", tag_value_arg(l, 1));
	function_pop(st);
	onion_block_free(d->code);
	d->code=NULL;
	
	function_add_code(st, "  %s(context, res);", d->id);
	d=(function_data*)st->function_stack->tail->data;
	d->flags|=F_NO_MORE_WRITE;
}

void tag_block(parser_status *st, list *l){
	const char *block_name=tag_value_arg(l, 1);
	function_add_code(st, 
"  {\n"
"    void (*f)(onion_dict *context, onion_response *res);\n"
"    f=(void*)onion_dict_get(context, \"__block_%s__\");\n"
"    if (f)\n"
"      f(context, res);\n"
"  }\n", block_name);

	char tmp[256];
	assert (strlen(st->infilename)<sizeof(tmp));
	strncpy(tmp, st->infilename, sizeof(tmp)-1);
	function_data *d=function_new(st, "%s__block_%s", basename(tmp),  block_name);
	function_add_code_f(st->blocks_init, 
"  if (!onion_dict_get(context, \"__block_%s__\"))\n"
"    onion_dict_add(context, \"__block_%s__\", %s, 0);\n", block_name, block_name, d->id);
}

void tag_endblock(parser_status *st, list *l){
	function_pop(st);
}
