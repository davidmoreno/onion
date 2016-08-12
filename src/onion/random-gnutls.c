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

#include <stdlib.h>
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>
#if GNUTLS_VERSION_NUMBER < 0x020C00
#include <gcrypt.h>
#endif
#include <assert.h>

#include "types_internal.h"
#include "log.h"

#ifdef HAVE_PTHREADS
#include <pthread.h>
static pthread_mutex_t onion_random_refcount_mutex = PTHREAD_MUTEX_INITIALIZER;
#define onion_random_refcount_mutex_lock() pthread_mutex_lock(&onion_random_refcount_mutex);
#define onion_random_refcount_mutex_unlock() pthread_mutex_unlock(&onion_random_refcount_mutex);
#else
#define onion_random_refcount_mutex_lock() ;
#define onion_random_refcount_mutex_unlock() ;
#endif

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
	onion_random_refcount_mutex_lock();
	if( onion_random_refcount == 0) {
		gnutls_global_init();
	}
	onion_random_refcount++;
	onion_random_refcount_mutex_unlock();
}

/**
 * @short Free up memory used by global random number generator
 *
 * onion_random_free() must not be called more times than onion_random_init()
 */
void onion_random_free() {
	onion_random_refcount_mutex_lock();
	assert( onion_random_refcount > 0 );
	onion_random_refcount--;
	if( onion_random_refcount == 0 ) {
		gnutls_global_deinit();
	}
	onion_random_refcount_mutex_unlock();
}

/**
 * @short Generates random data
 *
 * Generate size bytes of random data and put on data
 */
void onion_random_generate(void* data, size_t size) {
#if GNUTLS_VERSION_NUMBER >= 0x020C00
	gnutls_rnd(GNUTLS_RND_NONCE,data,size);
#else
	gcry_create_nonce(data, size);
#endif
}
