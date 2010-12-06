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

#ifndef __ONION_REQUEST_PARSER__
#define __ONION_REQUEST_PARSER__

#include "types.h"

#ifdef __cplusplus
extern "C"{
#endif

/// Used by req->parse_state, states are:
typedef enum parse_state_e{
	CLEAN=0,
	HEADERS=1,
	POST_DATA=2,
	POST_DATA_MULTIPART=3,  // On headers of multipart
	POST_DATA_MULTIPART_NOFILE=4,  // On multipart, normal post
	POST_DATA_MULTIPART_FILE=5,  // On multipart, a file
	POST_DATA_URLENCODE=6,
	FINISHED=100,
}parse_state;

#ifdef __cplusplus
}
#endif

#endif
