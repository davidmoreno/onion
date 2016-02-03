/*
	Onion HTTP server library
	Copyright (C) 2016 David Moreno Montero and others.

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

