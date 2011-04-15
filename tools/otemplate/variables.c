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
#include "block.h"
#include <string.h>

/**
 * @short Parses a block variable and writes the code necesary.
 * 
 * It can go deep inside a dict or list, and apply filters.
 * 
 * TODO. Just now only plain vars.
 */
void variable_write(parser_status *st, const char *b){
	
	function_add_code(st,
"  {\n"
"    const char *tmp;\n");
	variable_solve(st, b, "tmp", 0);
	function_add_code(st,
"    onion_response_write0(res, tmp);\n"
"  }\n");
}

/**
 * @short Solves a variable into code.
 * 
 * It uses the type to check it its a simple string. (type==1)
 */
void variable_solve(parser_status *st, const char *b, const char *tmpname, int type){
	if (type==1){
		function_add_code(st,
"    %s=\"%s\";\n", tmpname, b);
		return;
	}
	
	list *parts=list_new(block_free);
	block *lastblock;
	list_add(parts, lastblock=block_new());
	
	
	int i=0;
	int l=strlen(b);
	for (i=0;i<l;i++){
		if (b[i]=='.')
			list_add(parts, lastblock=block_new());
		else if (b[i]==' ')
			continue;
		else
			block_add_char(lastblock, b[i]);
	}

	if (list_count(parts)==1){
		block_safe_for_printf(lastblock);
		function_add_code(st, 
"    %s=onion_dict_get(context, \"%s\");\n", tmpname, lastblock->data);
	}
	else{
		function_add_code(st,
"    %s=NULL;\n"
"    {\n"
"      onion_dict *d=context;\n", tmpname);
		list_item *it=parts->head;
		while (it->next){
			lastblock=it->data;
			
			block_safe_for_printf(lastblock);
			function_add_code(st,
"      if (d)\n"
"        d=onion_dict_get_dict(d, \"%s\");\n", lastblock->data);
			it=it->next;
		}
		lastblock=it->data;
		
		block_safe_for_printf(lastblock);
		function_add_code(st, 
"      if (d)\n"
"        %s=onion_dict_get(d, \"%s\");\n", tmpname, lastblock->data);
		function_add_code(st, 
"    }\n");
	}
	list_free(parts);

	
}

