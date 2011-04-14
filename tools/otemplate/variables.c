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

void write_variable(parser_status *st, block *b){
	block_safe_for_printf(b);
	template_add_text(st, 
"  tmp=onion_dict_get(context, \"%s\");\n"
"  if (tmp)\n"
"    onion_response_write0(res, tmp);\n", b->data);
}

