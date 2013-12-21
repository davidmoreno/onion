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
#include <time.h>
#include <fcntl.h>
#include <assert.h>

#include "types_internal.h"
#include "log.h"

static unsigned onion_random_refcount=0;

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

	int fd=open("/dev/random", O_RDONLY);
	if (fd<0){
		ONION_WARNING("Unsecure random number generation; could not open /dev/random to feed the seed");
		// Just in case nobody elses do it... If somebody else do it, then no problem.
		srand(time(NULL));
	}
	else{
		unsigned int sr;
		read(fd, &sr, sizeof(sr));
		close(fd);
		srand(sr);
	}
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

	//rand() based implementatio does nothing actually, just checks if refcount is ok
}

/**
 * @short Generates random data
 *
 * Generate size bytes of random data and put on data
 */
void onion_random_generate(void* data, unsigned int size) {
	unsigned char* data_char = data;
	unsigned int i;
	for( i=0;i<size;++i ) {
		data_char[i] = rand();
	}
}
