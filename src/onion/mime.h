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

#ifndef ONION_MIME_H
#define ONION_MIME_H

#ifdef __cplusplus
extern "C"{
#endif

#include "types.h"

/// Sets the mime dictionary (extension -> mime_type)
void onion_mime_set(onion_dict *);
/// Returns a mime type based on the file name.
const char *onion_mime_get(const char *filename);
/// Updates a mime record, for that extensions set that mimetype. If mimetype==NULL, removes it.
void onion_mime_update(const char *extension, const char *mimetype);

#ifdef __cplusplus
}
#endif



#endif

