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

#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <onion/log.h>
#include <onion/block.h>

#include "list.h"
#include "parser.h"

int use_orig_line_numbers=1;

/// Writes to st->out the declarations of the functions for this template
void functions_write_declarations(parser_status *st){
	list_item *it=st->functions->head;
	while (it){
		function_data *d=it->data;
		if (d->is_static)
			fprintf(st->out, "static ");
		fprintf(st->out, "void %s(%s);\n", d->id, d->signature ? d->signature : "onion_dict *context, onion_response *res");
		it=it->next;
	}
}

/// Writes the desired function to st->out
static void function_write(parser_status *st, function_data *d){
	if (d->code){
		if (use_orig_line_numbers)
			fprintf(st->out, "#line 0\n");
		if (d->is_static)
			fprintf(st->out, "static ");
		fprintf(st->out, 
"void %s(%s){\n", d->id, d->signature ? d->signature : "onion_dict *context, onion_response *res"
					);
		
		if (use_orig_line_numbers){
			fprintf(st->out, "#line 0\n");
		
			// Write code, but change \n\n to \n
			const char *data=onion_block_data(d->code);
			int ldata=onion_block_size(d->code);
			int i=0, li=0;
			char lc=0;
			for (i=0;i<ldata;i++){
				if (data[i]=='\n' && lc=='\n'){ // Two in a row
					fwrite(&data[li], 1, i-li-1, st->out);
					li=i;
				}
				lc=data[i];
			}
			fwrite(&data[li], 1, i-li, st->out);
			fprintf(st->out, "#line 0\n");
		}
		else{
			fwrite(onion_block_data(d->code), 1, onion_block_size(d->code), st->out);
		}

		fprintf(st->out,"}\n");
	}
}

/// Writes all functions to st->out
void functions_write_code(parser_status *st){
	list_loop(st->functions, function_write, st);
}

/// Writes the main function code.
void functions_write_main_code(parser_status *st){
	const char *f=((function_data*)list_get_n(st->function_stack,1))->id;

		fprintf(st->out,"\n\n"
"int %s_handler_page(onion_dict *context, onion_request *req, onion_response *res){\n"
"\n"
"  %s(context, res);\n"
"\n"
"  return OCS_PROCESSED;\n"
"}\n\n", f, f);

	fprintf(st->out,
"\n"
"onion_handler *%s_handler(onion_dict *context){\n"
"  return onion_handler_new((onion_handler_handler)%s_handler_page, context, (onion_handler_private_data_free)onion_dict_free);\n"
"}\n\n", f, f);
	

	fprintf(st->out,
"\n"
"int %s_template(onion_dict *context, onion_request *req, onion_response *res){\n"
"\n"
"  if (context) onion_dict_add(context, \"LANG\", onion_request_get_language_code(req), 0);\n"
"\n"
"  %s(context, res);\n"
"\n"
"  if (context) onion_dict_free(context);\n"
"\n"
"  return OCS_PROCESSED;\n"
"}\n\n", f, f);
}

/**
 * @short Creates a new function on top of the stack.
 * 
 * It can receive a fmt argument that sets the id. If none, its assigned automatically.
 */
function_data *function_new(parser_status *st, const char *fmt, ...){
	function_data *d=malloc(sizeof(function_data));
	d->flags=0;
	d->code=onion_block_new();
	if (st){
		st->current_code=d->code;
		list_add(st->function_stack, d);
		list_add(st->functions, d);
		//ONION_DEBUG("push function stack, length is %d", list_count(st->function_stack));
	}
	d->signature=NULL;
	
	char tmp[64];
	if (!fmt){
		d->is_static=1;
		snprintf(tmp, sizeof(tmp), "otemplate_f_%04X",st->function_count++);
	}
	else{
		d->is_static=0;
		va_list va;
		va_start(va, fmt);
		vsnprintf(tmp,sizeof(tmp),fmt,va);
		va_end(va);
		
		char *p=tmp;
		while (*p){
			if (*p=='.')
				*p='_';
			p++;
		}
	}
	
	//ONION_DEBUG("New function %s", tmp);
	
	
	d->id=strdup(tmp);
	return d;
}


/**
 * @short Removes the data used by the function.
 */
void function_free(function_data *d){
	if (d->code)
		onion_block_free(d->code);
	free(d->id);
	free(d);
}


/**
 * @short Pops the function from the stack. Its still at the function list.
 */
function_data *function_pop(parser_status *st){
	function_data *p=(function_data*)st->function_stack->tail->data;
	list_pop(st->function_stack);
	//ONION_DEBUG("pop function stack, length is %d", list_count(st->function_stack));
	st->current_code=((function_data*)st->function_stack->tail->data)->code;
	return p;
}


/**
 * @short Adds some code to the top function
 */
void function_add_code(parser_status *st, const char *fmt, ...){
	function_data *p=(function_data*)st->function_stack->tail->data;
	if (p->flags&F_NO_MORE_WRITE)
		return;
	
	char tmp[4096];
	
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(tmp, sizeof(tmp), fmt, ap);
	va_end(ap);

	if (use_orig_line_numbers){
		char line[32];
		int p=onion_block_size(st->current_code);
		if (p && onion_block_data(st->current_code)[p-1]!='\n')
			onion_block_add_char(st->current_code, '\n');
		snprintf(line,sizeof(line),"#line %d\n", st->line);
		// I have to do it for every \n too. This is going to be slow.
		const char *orig=tmp;
		int lorig=strlen(orig);
		int i=0, li=0;
		for (i=0;i<lorig;i++){
			if (orig[i]=='\n'){
				onion_block_add_str(st->current_code, line);
				onion_block_add_data(st->current_code, &orig[li], i-li+1);
				li=i;
			}
		}
		if (i-1!=li){
			onion_block_add_str(st->current_code, line);
			onion_block_add_str(st->current_code, &orig[li]);
		}
	}
	else{
		//ONION_DEBUG("Add to level %d text %s",list_count(st->function_stack), tmp);
		onion_block_add_str(st->current_code, tmp);
	}
}

/**
 * @short Adds some code to a given function.
 */
void function_add_code_f(struct function_data_t *f, const char *fmt, ...){
	char tmp[4096];
	
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(tmp, sizeof(tmp), fmt, ap);
	va_end(ap);
	
	//ONION_DEBUG("Add to level %d text %s",list_count(st->function_stack), tmp);
	
	onion_block_add_str(f->code, tmp);
}
