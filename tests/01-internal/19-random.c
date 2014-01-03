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

#include <onion/onion.h>
#include <onion/log.h>
#include <onion/random.h>
#include <string.h>
#include "../ctest.h"

// this is a simple test to check the implementation, not the algorithm
void t01_test_random(){
	INIT_LOCAL();
	
	onion_random_init();

	unsigned char rand_generated[256];
	onion_random_generate(rand_generated,256);

	//set of which numbers were generated
	char char_set[256];
	memset(char_set,0,256);

	unsigned unique_numbers_generated=0;
	unsigned i;
	for( i=0; i<256; ++i ) {
		if( char_set[ rand_generated[i] ] == 0) {
			char_set[ rand_generated[i] ] = 1;
			unique_numbers_generated++;
		}
	}

	FAIL_IF_NOT( unique_numbers_generated > 256/2 );

	onion_random_free();

	END_LOCAL();
}

int main(int argc, char **argv){
  START();
  
	t01_test_random();
	
	END();
}
