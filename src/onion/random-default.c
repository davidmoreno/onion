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
#include <time.h>
#include <fcntl.h>
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
	if( onion_random_refcount == 0 ) {
		int fd=open("/dev/random", O_RDONLY);
		if (fd<0){
			ONION_WARNING("Unsecure random number generation; could not open /dev/random to feed the seed");
			// Just in case nobody elses do it... If somebody else do it, then no problem.
			srand(time(NULL));
		}
		else{
			unsigned int sr;
			ssize_t n;
			n = read(fd, &sr, sizeof(sr));
			if (n != sizeof(sr)) {
				ONION_ERROR("Error reading seed value from /dev/random after file descriptor opened");
				exit(1);
			} else {
				close(fd);
				srand(sr);
			}
		}
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
	if( onion_random_refcount > 0 ) {
		onion_random_refcount_mutex_unlock();
		return;
	}
	onion_random_refcount_mutex_unlock();

	//rand() based implementatio does nothing actually, just checks if refcount is ok
}

/**
 * @short Generates random data
 *
 * Generate size bytes of random data and put on data
 */
void onion_random_generate(void* data, size_t size) {
	unsigned char* data_char = data;
	size_t i;
	for( i=0;i<size;++i ) {
		data_char[i] = rand();
	}
}
