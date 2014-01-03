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

#include <onion/mime.h>
#include "../ctest.h"

void t01_test_mime(){
	INIT_LOCAL();
	
	FAIL_IF_NOT_EQUAL_STR("text/plain", onion_mime_get("txt"));
	FAIL_IF_NOT_EQUAL_STR("text/plain", onion_mime_get("fdsfds"));
	FAIL_IF_NOT_EQUAL_STR("text/html", onion_mime_get("html"));
	FAIL_IF_NOT_EQUAL_STR("image/png", onion_mime_get("file.png"));
	FAIL_IF_NOT_EQUAL_STR("application/javascript", onion_mime_get("js"));

	
	onion_mime_set(NULL);
	END_LOCAL();
}


int main(int argc, char **argv){
	START();
	
	t01_test_mime();
	
	END();
}
