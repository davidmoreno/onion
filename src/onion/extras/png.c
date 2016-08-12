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

#include <png.h>
#include <onion/response.h>
#include <onion/log.h>

struct onion_png_data_t{
	onion_response *res;
	int sent;
};

typedef struct onion_png_data_t onion_png_data;

// If not sent yet, writes the error. also always write it to the log.
static void error(png_struct *p, const char *msg){
	onion_png_data *d=(onion_png_data*)png_get_io_ptr(p);
	if (!d->sent){
		onion_response_set_code(d->res, HTTP_INTERNAL_ERROR);
		onion_response_printf(d->res, "Libpng error: %s", msg);
		d->sent=1;
	}
	
	ONION_ERROR("%s", msg);
}

// Shows a warning on the logs
static void warning(png_struct *p, const char *msg){
	ONION_WARNING("%s", msg);
} 

// Writes the data to the response
static void onion_png_write(png_struct *p, png_bytep data, size_t l){
	onion_png_data *d=(onion_png_data*)png_get_io_ptr(p);
	onion_response_write(d->res, (const char *)data, l);
	//ONION_DEBUG("Write");
}

// do nothing.
static void onion_png_flush(png_struct *p){
	//ONION_DEBUG("Flush");
}

/**
 * @short Writes an image as png to the response object
 * 
 * @param image flat buffer with all pixels
 * @param Bpp Bytes per pixel: 1 grayscale, 2 grayscale with alpha, 3 RGB, 4 RGB with alpha. Negative if in BGR format (cairo)
 * @param width The width of the image
 * @param height The height of the image
 * @param res where to write the image, it sets the necessary structs
 */
int onion_png_response(unsigned char *image, int Bpp, int width, int height, onion_response *res){
	// Many copied from example.c from libpng source code.
	png_structp png_ptr;
	png_infop info_ptr;

	/* Create and initialize the png_struct with the desired error handler
	* functions.  If you want to use the default stderr and longjump method,
	* you can supply NULL for the last three parameters.  We also check that
	* the library version is compatible with the one used at compile time,
	* in case we are using dynamically linked libraries.  REQUIRED.
	*/
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, error, warning);

	if (png_ptr == NULL)
	{
		return OCS_INTERNAL_ERROR;
	}

	/* Allocate/initialize the image information data.  REQUIRED */
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
		png_destroy_write_struct(&png_ptr,  NULL);
		return OCS_INTERNAL_ERROR;
	}

	onion_png_data opd;
	opd.res=res;
	opd.sent=0;
	png_set_write_fn(png_ptr, (void *)&opd, onion_png_write, onion_png_flush);
	/* where user_io_ptr is a structure you want available to the callbacks */

	onion_response_set_header(res, "Content-Type", "image/png");
	if (onion_response_write_headers(res)==OR_SKIP_CONTENT) // Maybe it was HEAD.
		return OCS_PROCESSED;
	 
	/* Set the image information here.  Width and height are up to 2^31,
	* bit_depth is one of 1, 2, 4, 8, or 16, but valid values also depend on
	* the color_type selected. color_type is one of PNG_COLOR_TYPE_GRAY,
	* PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COLOR_TYPE_PALETTE, PNG_COLOR_TYPE_RGB,
	* or PNG_COLOR_TYPE_RGB_ALPHA.  interlace is either PNG_INTERLACE_NONE or
	* PNG_INTERLACE_ADAM7, and the compression_type and filter_type MUST
	* currently be PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE. REQUIRED
	*/
	if (Bpp<0){
		png_set_bgr(png_ptr);
		Bpp=-Bpp;
	}

	switch(Bpp){
		case 1:
			png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_GRAY,
					PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
		break;
		case 2:
			png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_GRAY_ALPHA,
					PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
		break;
		case 3:
			png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB,
					PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
		break;
		case 4:
			png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB_ALPHA,
					PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
		break;
		default:
			png_error(png_ptr, "Wrong bytes per pixel");
			break;
	}
	
	png_uint_32 k;
	png_bytep row_pointers[height];

	if (height > PNG_UINT_32_MAX/sizeof(png_bytep))
		png_error (png_ptr, "Image is too tall to process in memory");

	for (k = 0; k < height; k++)
		row_pointers[k] = (png_byte *) (image + k*width*Bpp);

	// Sets the rows to save at the image
	png_set_rows(png_ptr, info_ptr, row_pointers);
	 
	// If header already sent, then there was an error.
	if (opd.sent){
		return OCS_PROCESSED;
	}
	opd.sent=1;
	// Finally WRITE the image. Uses the onion_response_write via onion_png_write helper.
	png_write_png(png_ptr, info_ptr, 0, NULL);
	
	/* Clean up after the write, and free any memory allocated */
	png_destroy_write_struct(&png_ptr, &info_ptr);

	/* That's it */
	return OCS_PROCESSED;
}

