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

#include "block.h"

typedef enum parser_mode_e{
	TEXT=0,
	TAG=2,
	CODE=3,
	
	// transitional modes
	BEGIN=17,
	END_TAG=18,
	END_CODE=19,
}parser_mode;

struct function_data_t{
	char *id;
	block *code; /// Blocks of C code to be printed.
};

typedef struct function_data_t function_data;


struct parser_status_t{
	parser_mode mode;
	FILE *in;
	FILE *out;
	
	block *rawblock; /// Current block of data as readed by the parser.
	
	list *functions; /// List of all known functions
	list *function_stack; /// Current stack of functions, to know where to write.
	block *current_code; /// Blocks of C code to be printed. this is final code.
	char c; /// Last character read
	int status; /// Exit status.
};
typedef struct parser_status_t parser_status;


void parse_template(parser_status *status);
void set_mode(parser_status *status, int mode);

void add_char(parser_status *st, char c);
void write_block(parser_status *st, block *b);

void write_code(parser_status *st, block *b);


void write_other_functions_declarations(parser_status *st);
void write_main_function(parser_status *st);
void write_other_functions(parser_status *st);
void write_function_call(parser_status *st, function_data *f);
void write_function(parser_status *st, function_data *d);

function_data *function_new(parser_status *st);
void function_free(function_data *d);


function_data *parser_stack_pop(parser_status *st);
void parser_add_text(parser_status *st, const char *fmt, ...);
