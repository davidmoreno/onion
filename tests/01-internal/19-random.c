/**
  Onion HTTP server library
  Copyright (C) 2010-2018 David Moreno Montero and others

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

  You should have received a copy of both licenses, if not see
  <http://www.gnu.org/licenses/> and
  <http://www.apache.org/licenses/LICENSE-2.0>.
*/

#include <onion/onion.h>
#include <onion/log.h>
#include <onion/random.h>
#include <string.h>
#include "../ctest.h"

// this is a simple test to check the implementation, not the algorithm
void t01_test_random() {
  INIT_LOCAL();

  onion_random_init();

  unsigned char rand_generated[256];
  onion_random_generate(rand_generated, 256);

  //set of which numbers were generated
  char char_set[256];
  memset(char_set, 0, 256);

  unsigned unique_numbers_generated = 0;
  unsigned i;
  for (i = 0; i < 256; ++i) {
    if (char_set[rand_generated[i]] == 0) {
      char_set[rand_generated[i]] = 1;
      unique_numbers_generated++;
    }
  }

  FAIL_IF_NOT(unique_numbers_generated > 256 / 2);

  onion_random_free();

  END_LOCAL();
}

int main(int argc, char **argv) {
  START();

  t01_test_random();

  END();
}
