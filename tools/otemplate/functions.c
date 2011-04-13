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

#include <onion/log.h>

#include "list.h"
#include "parser.h"
#include "block.h"

void write_other_functions_declarations(parser_status *st){
	list_item *it=st->functions->head;
	while (it){
		function_data *d=it->data;
		fprintf(st->out, "void %s(onion_dict *context, onion_response *res);\n", d->id);
		it=it->next;
	}
}

/**
 * @short Writes a call to the main function.
 * 
 * The main function is the first at the stack.
 */
void write_main_function(parser_status *st){
	write_function_call(st, st->function_stack->head->data);
	
}

void write_function_call(parser_status *st, function_data *f){
	fprintf(st->out, "  %s(context, res);\n",f->id);
}

void write_other_functions(parser_status *st){
	list_loop(st->functions, write_function, st);
}

void write_function(parser_status *st, function_data *d){
	fprintf(st->out, 
"void %s(onion_dict *context, onion_response *res){\n"
"  const char *tmp=NULL;\n"
"  tmp=tmp;\n" // avoid compiled complain about not using it. I dont know yet.
"\n", d->id);
	
	fwrite(d->code->data, 1, d->code->pos, st->out);
	
	fprintf(st->out,"\n}\n\n");
}


function_data *function_new(parser_status *st){
	function_data *d=malloc(sizeof(function_data));
	d->code=block_new(NULL);
	if (st){
		st->current_code=d->code;
		list_add(st->function_stack, d);
		list_add(st->functions, d);
		ONION_DEBUG("push function stack, length is %d", list_count(st->function_stack));
	}
	
	char tmp[64];
	snprintf(tmp, sizeof(tmp), "otemplate_f_%X",rand());

	ONION_DEBUG("New function %s", tmp);
	
	
	d->id=strdup(tmp);
	return d;
}

void function_free(function_data *d){
	block_free(d->code);
	free(d->id);
	free(d);
}
