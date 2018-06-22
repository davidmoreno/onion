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
#include "../ctest.h"
#include "mime.hpp"

void t01_basic() {
	INIT_LOCAL();

	FAIL_IF_NOT_EQUAL_STRING(Onion::Mime::get(".html"), "text/html");

	Onion::Mime::remove("html");
	FAIL_IF_NOT_EQUAL_STRING(Onion::Mime::get(".html"), "text/plain");

	Onion::Mime::update("html", "application/html");
	FAIL_IF_NOT_EQUAL_STRING(Onion::Mime::get(".html"), "application/html");

	Onion::Mime::remove("html");
	Onion::Mime::update("html", "text/html");
	END_LOCAL();
}

void t02_backing_dict() {
	INIT_LOCAL();

	Onion::Dict dict;
	dict.add("crazy", "application/x-crazy-mime-type");

	Onion::Mime::set(dict);

	FAIL_IF_NOT_EQUAL_STRING(Onion::Mime::get(".crazy"), "application/x-crazy-mime-type");
	END_LOCAL();
}

int main(int argc, char **argv) {
	START();
	t01_basic();
	t02_backing_dict();
	END();
}

