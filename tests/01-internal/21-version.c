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

#include "../ctest.h"
#include <onion/version.h>

void t01_current_version(){
  INIT_LOCAL();

  FAIL_IF_NOT_EQUAL_STR(ONION_VERSION, onion_version());
  FAIL_IF_NOT_EQUAL_INT(ONION_VERSION_MAJOR, onion_version_major());
  FAIL_IF_NOT_EQUAL_INT(ONION_VERSION_MINOR, onion_version_minor());
  FAIL_IF_NOT_EQUAL_INT(ONION_VERSION_PATCH, onion_version_patch());
  FAIL_IF_NOT(ONION_VERSION_IS_COMPATIBLE());
  
  ONION_VERSION_IS_COMPATIBLE_OR_ABORT();

  // Patch version is not important
  FAIL_IF_NOT(onion_version_is_compatible3(ONION_VERSION_MAJOR,ONION_VERSION_MINOR,0));
  FAIL_IF_NOT(onion_version_is_compatible3(ONION_VERSION_MAJOR,ONION_VERSION_MINOR,1));
  FAIL_IF_NOT(onion_version_is_compatible3(ONION_VERSION_MAJOR,ONION_VERSION_MINOR,1234));
  FAIL_IF_NOT(onion_version_is_compatible3(ONION_VERSION_MAJOR,ONION_VERSION_MINOR,1000));

  END_LOCAL();
}

void t02_older_version(){
  INIT_LOCAL();

  FAIL_IF(onion_version_is_compatible3(ONION_VERSION_MAJOR-1,0,0));
  FAIL_IF_NOT(onion_version_is_compatible3(ONION_VERSION_MAJOR,0,0));
  FAIL_IF_NOT(onion_version_is_compatible3(ONION_VERSION_MAJOR,ONION_VERSION_MINOR-1,0));
  FAIL_IF_NOT(onion_version_is_compatible3(ONION_VERSION_MAJOR,ONION_VERSION_MINOR-1,1000));

  END_LOCAL();
}

void t03_newer_version(){
  INIT_LOCAL();

  FAIL_IF_NOT(onion_version_is_compatible3(ONION_VERSION_MAJOR, ONION_VERSION_MINOR, 0));
  FAIL_IF_NOT(onion_version_is_compatible3(ONION_VERSION_MAJOR, ONION_VERSION_MINOR, ONION_VERSION_PATCH+1));
  FAIL_IF_NOT(onion_version_is_compatible3(ONION_VERSION_MAJOR, ONION_VERSION_MINOR, ONION_VERSION_PATCH+1000));

  FAIL_IF(onion_version_is_compatible3(ONION_VERSION_MAJOR+1, ONION_VERSION_MINOR, ONION_VERSION_PATCH));
  FAIL_IF(onion_version_is_compatible3(ONION_VERSION_MAJOR+1000, ONION_VERSION_MINOR, ONION_VERSION_PATCH));

  // Cant use older minor versions, maybe new symbols
  FAIL_IF(onion_version_is_compatible3(ONION_VERSION_MAJOR, ONION_VERSION_MINOR+1, ONION_VERSION_PATCH+1000));
  FAIL_IF(onion_version_is_compatible3(ONION_VERSION_MAJOR, ONION_VERSION_MINOR+1000, ONION_VERSION_PATCH+1000));

  END_LOCAL();
}

int main(int argc, char **argv){
  START();

  t01_current_version();
  t02_older_version();
  t03_newer_version();

  END();
}
