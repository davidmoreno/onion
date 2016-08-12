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
#ifndef ONION_UTILS_H
#define ONION_UTILS_H

#ifdef __cplusplus
extern "C"{
#endif

// In the HTTP RFC whitespace is always these characters
// and is not locale independent, we'll need this when
// parsing
static int __attribute__ ((unused)) is_space(char c) {
	if(c == '\t' || c == '\n' || c == '\r' || c == ' ')
		return 1;
	return 0;
}
static int __attribute__ ((unused)) is_alnum(char c) {
	if((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
		return 1;
	return 0;
}

#ifdef __cplusplus
}
#endif


#endif
