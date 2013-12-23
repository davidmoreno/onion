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

#include <stdlib.h>
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>
#include <assert.h>

#include "types_internal.h"
#include "log.h"

static size_t onion_random_refcount=0;

/**
 * @short Initializes the global random number generator
 * 
 * Initializes the global random number generator with some random seed.
 *
 * onion_random_free() must be called later to free up used memory.
 *
 * It is safe to call onion_random_init() more than once, but union_random_free() must be called the same amount of times.
 */ 
void onion_random_init() {
	onion_random_refcount++;
	if( onion_random_refcount > 1 )
		return;

	gnutls_global_init();
}

/**
 * @short Free up memory used by global random number generator
 *
 * onion_random_free() must not be called more times than onion_random_init()
 */
void onion_random_free() {
	assert( onion_random_refcount > 0 );
	onion_random_refcount--;
	if( onion_random_refcount > 0 )
		return;

	gnutls_global_deinit();
}

/**
 * @short Generates random data
 *
 * Generate size bytes of random data and put on data
 */
void onion_random_generate(void* data, size_t size) {
	gnutls_rnd(GNUTLS_RND_NONCE,data,size);
}
