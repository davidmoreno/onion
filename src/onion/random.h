/*
	Onion HTTP server library
	Copyright (C) 2010-2013 David Moreno Montero

	This library is free software; you can redistribute it and/or
	modify it under the terms of, at your choice:
	
	a. the GNU Lesser General Public License as published by the 
	 Free Software Foundation; either version 3.0 of the License, 
	 or (at your option) any later version.
	
	b. the GNU General Public License as published by the 
	 Free Software Foundation; either version 2.0 of the License, 
	 or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License and the GNU General Public License along with this 
	library; if not see <http://www.gnu.org/licenses/>.
	*/

#ifndef ONION_RANDOM_H
#define ONION_RANDOM_H

#include "types.h"

#ifdef __cplusplus
extern "C"{
#endif

/// Initializes the global random number generator
void onion_random_init();

/// Frees the global random number generator
void onion_random_free();

/// Generate size bytes of random data and put on data
void onion_random_generate(void* data, size_t size);

#ifdef __cplusplus
}
#endif

#endif
