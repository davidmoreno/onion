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

#ifndef __FUNCTIONS_H__
#define __FUNCTIONS_H__

#include <onion/types.h>
#include "../common/updateassets.h"

enum function_data_flags_e{
	F_NO_MORE_WRITE=1,
};

typedef enum function_data_flags_e function_data_flags;

struct function_data_t{
	char *id;
	onion_block *code; /// Blocks of C code to be printed.
	int is_static:1;
	const char *signature; /// The function signature. If NULL its the standard
	int flags;
};

typedef struct function_data_t function_data;

struct parser_status_t;

void functions_write_declarations_assets(struct parser_status_t *st, onion_assets_file *assets);
void functions_write_declarations(struct parser_status_t *st);
void functions_write_code(struct parser_status_t *st);
void functions_write_main_code(struct parser_status_t *st);


function_data *function_new(struct parser_status_t *st, const char *fmt, ...);
void function_free(function_data *d);
function_data *function_pop(struct parser_status_t *st);
void function_add_code(struct parser_status_t *st, const char *fmt, ...);
void function_add_code_f(struct function_data_t *f, const char *fmt, ...);

/// Whether to add #line directives so debugging of otemplates is easier.
extern int use_orig_line_numbers;

#endif
