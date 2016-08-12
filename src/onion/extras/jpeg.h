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

#ifndef __ONION_JPEG_H__
#define __ONION_JPEG_H__

#ifdef __cplusplus
extern "C"{
#endif

#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <jpeglib.h>
#include <onion/types.h>

/// Writes image data to a response object
int onion_jpeg_response ( unsigned char * image,
		int image_num_color_channels,
		J_COLOR_SPACE image_color_space, /* See jpeglib.h for list of available color spaces. */
		int image_width,
		int image_height,
		int output_quality,
		onion_response *res
		);

#ifdef __cplusplus
}
#endif

#endif
