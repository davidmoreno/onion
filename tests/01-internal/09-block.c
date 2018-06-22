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

#include <onion/block.h>
#include <onion/log.h>

#include "../ctest.h"

void t01_create_and_free() {
  INIT_TEST();

  onion_block *block = onion_block_new();

  FAIL_IF_EQUAL(block, NULL);

  int i;
  for (i = 0; i < 16 * 1024; i++) {
    onion_block_add_char(block, (char)i);
  }

  onion_block_free(block);

  END_TEST();
}

void t02_several_add_methods() {
  INIT_TEST();

  onion_block *block = onion_block_new();

  FAIL_IF_EQUAL(block, NULL);

  int i;
  for (i = 0; i < 1024; i++) {
    onion_block_add_char(block, (char)i);
  }
  onion_block_clear(block);

  onion_block_add_str(block, "first ");
  for (i = 0; i < 1024; i++)
    onion_block_add_str(block, "test ");

  FAIL_IF_NOT_STRSTR(onion_block_data(block), "test");

  for (i = 0; i < 1024; i++)
    onion_block_add_data(block, "world", 4);

  FAIL_IF_STRSTR(onion_block_data(block), "world");
  FAIL_IF_NOT_STRSTR(onion_block_data(block), "worl");

  int s = onion_block_size(block);

  onion_block_add_block(block, block);
  FAIL_IF_NOT_EQUAL(onion_block_size(block), s + s);

  onion_block_free(block);

  END_TEST();
}

int main(int argc, char **argv) {
  START();

  t01_create_and_free();
  t02_several_add_methods();

  END();
}
