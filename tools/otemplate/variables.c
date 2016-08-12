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

#include "parser.h"
#include "variables.h"
#include <string.h>
#include <onion/block.h>
#include <onion/codecs.h>
#include <onion/log.h>
#include <stdlib.h>

/**
 * @short Parses a block variable and writes the code necesary.
 * 
 * It can go deep inside a dict or list, and apply filters.
 */
void variable_write(parser_status *st, onion_block *b){
	
	function_add_code(st,
"  {\n"
"    const char *tmp;\n");
	variable_solve(st, onion_block_data(b), "tmp", STRING);
	function_add_code(st,
"    if (tmp)\n"
"      onion_response_write_html_safe(res, tmp);\n"
"  }\n");
}

/**
 * @short Solves a variable into code.
 * 
 * It uses the type to check it its a literal string, a dcit string or a dict.
 */
void variable_solve(parser_status *st, const char *data, const char *tmpname, vartype_e type){
	if (type==LITERAL){
		char *s=onion_c_quote_new(data);
		function_add_code(st,
"    %s=%s;\n", tmpname, s);
		free(s);
		return;
	}
	if (! (type==STRING || type==DICT) ){
		ONION_ERROR("Invalid type for variable solve");
		exit(1);
	}
	
	
	list *parts=list_new(onion_block_free);
	onion_block *lastblock;
	list_add(parts, lastblock=onion_block_new());
	
	
	int i=0;
	int l=strlen(data);
	const char *d=data;
	for (i=0;i<l;i++){
		if (d[i]=='.')
			list_add(parts, lastblock=onion_block_new());
		else if (d[i]==' ')
			continue;
		else
			onion_block_add_char(lastblock, d[i]);
	}

	if (list_count(parts)==1){
		char *s=onion_c_quote_new(onion_block_data(lastblock));
		if (type==STRING)
			function_add_code(st, 
	"    %s=onion_dict_get(context, %s);\n", tmpname, s);
		else if (type==DICT)
			function_add_code(st, 
	"    %s=onion_dict_get_dict(context, %s);\n", tmpname, s);
		free(s);
	}
	else{
		if (type==STRING)
			function_add_code(st,"    %s=onion_dict_rget(context", tmpname);
		else if (type==DICT)
			function_add_code(st,"    %s=onion_dict_rget_dict(context", tmpname);
		else{
			ONION_ERROR("Invalid type for variable solve");
			exit(1);
		}
		list_item *it=parts->head;
		while (it){
			lastblock=it->data;
			char *s=onion_c_quote_new(onion_block_data(lastblock));
			function_add_code(st,", %s", s);
			free(s);
			it=it->next;
		}
		function_add_code(st,", NULL);\n");
	}
	list_free(parts);
}

