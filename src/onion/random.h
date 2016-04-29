/*
	Onion HTTP server library
	Copyright (C) 2010-2016 David Moreno Montero and others

	This library is free software; you can redistribute it and/or
	modify it under the terms of, at your choice:

	a. the Apache License Version 2.0.

	b. the GNU General Public License as published by the
		Free Software Foundation; either version 2.0 of the License,
		or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of both libraries, if not see
	<http://www.gnu.org/licenses/> and
	<http://www.apache.org/licenses/LICENSE-2.0>.
	*/

#ifndef ONION_RANDOM_H
#define ONION_RANDOM_H

#include "types.h"

#ifdef __cplusplus
extern "C"{
#endif

/// @defgroup random Random. Cryptographically secure random generators.

/// Initializes the global random number generator
/// @ingroup random
void onion_random_init();

/// Frees the global random number generator
/// @ingroup random
void onion_random_free();

/// Generate size bytes of random data and put on data
/// @ingroup random
void onion_random_generate(void* data, size_t size);

#ifdef __cplusplus
}
#endif

#endif
