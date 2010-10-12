/*
	Onion HTTP server library
	Copyright (C) 2010 David Moreno Montero

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

#ifndef __ONION_DICT__
#define __ONION_DICT__

#ifdef __cplusplus
extern "C"{
#endif

/**
 * @short A 'char *' to 'char *' dicitonary.
 *
 * Internally its structured as a binary tree.
 */
struct onion_dict_t{
	const char *key;
	const char *value;
	char flags;
	struct onion_dict_t *left;
	struct onion_dict_t *right;
};

/**
 * @short Flags to change some parameters of each key.
 */
enum onion_dict_flags_e{
	OD_EMPTY=1,
	OD_FREE_KEY=2,     /// Whether the key has to be removed at free time
	OD_FREE_VALUE=4,   /// Whether the value has to be removed at free time
	OD_FREE_ALL=6,     /// Whether both, the key and value have to be removed at free time. In any case its also marked for freeing later.
	OD_DUP_KEY=0x12,   /// Whether the key has to be dupped
	OD_DUP_VALUE=0x24, /// Whether the value has to be dupped
	OD_DUP_ALL=0x36,   /// Whether both, the key and value have to be dupped. In any case its also marked for freeing later.
};

/// Wrapper to make it easier to use
typedef struct onion_dict_t onion_dict;

/// Initializes a dict.
onion_dict *onion_dict_new();

/// Adds a value
void onion_dict_add(onion_dict *dict, const char *key, const char *value, int flags);

/// Removes a value
int onion_dict_remove(onion_dict *dict, const char *key);

/// Removes the full dict struct form mem.
void onion_dict_free(onion_dict *dict);

/// Gets a value
const char *onion_dict_get(onion_dict *dict, const char *key);

/// Prints a dot ready graph to stderr
void onion_dict_print_dot(onion_dict *dict);

#ifdef __cplusplus
}
#endif

#endif
