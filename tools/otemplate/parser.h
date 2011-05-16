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

#ifndef __PARSER_H__
#define __PARSER_H__

#include <stdio.h>

#include <onion/types.h>

#include "functions.h"
#include "list.h"

typedef enum parser_mode_e{
	TEXT=0,
	VARIABLE=2,
	TAG=3,
	
	END=15,
	// transitional modes
	BEGIN=17,
	END_VARIABLE=18,
	END_TAG=19,
	
}parser_mode;


struct parser_status_t{
	parser_mode last_wmode;
	parser_mode mode;
	FILE *in;
	FILE *out;
	
	const char *infilename; /// Needed for includes using relative path
	
	onion_block *rawblock; /// Current block of data as readed by the parser.
	
	list *functions; /// List of all known functions
	list *function_stack; /// Current stack of functions, to know where to write.
	function_data *blocks_init; /// List of block names
	onion_block *current_code; /// Blocks of C code to be printed. this is final code.
	char c; /// Last character read
	int status; /// Exit status.
	int function_count;
	int line;
};
typedef struct parser_status_t parser_status;

void parse_template(parser_status *status);

void add_char(parser_status *st, char c);
void write_block(parser_status *st, onion_block *b);


#endif
