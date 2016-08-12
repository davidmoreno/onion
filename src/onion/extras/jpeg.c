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

#include <setjmp.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <jpeglib.h>

#include <onion/types_internal.h>
#include <onion/response.h>
#include <onion/log.h>

struct onion_jpeg_data_t{
	onion_response *res;
	int sent;
};

typedef struct onion_jpeg_data_t onion_jpeg_data;

struct onion_error_mgr {
	struct jpeg_error_mgr pub;    /* "public" fields */

	jmp_buf setjmp_buffer;        /* for return to caller */
};

typedef struct onion_error_mgr * onion_error_ptr;

/*
 * Here's the routine that will replace the standard error_exit method:
 */
void onion_error_exit (j_common_ptr cinfo)
{
	/* cinfo->err really points to a onion_error_mgr struct, so coerce pointer */
	onion_error_ptr myerr = (onion_error_ptr) cinfo->err;

	/* Always display the message. */
	/* We could postpone this until after returning, if we chose. */
	//(*cinfo->err->output_message) (cinfo);
	/* Create the message */
	char* msg = (char *) malloc( JMSG_LENGTH_MAX);
	( *(cinfo->err->format_message) ) (cinfo, msg);

	onion_jpeg_data *d=(onion_jpeg_data*)cinfo->client_data;
	if (!d->sent){
		onion_response_set_code(d->res, HTTP_INTERNAL_ERROR);
		onion_response_printf(d->res, "Libpng error: %s", msg);
		d->sent=1;
	}

	ONION_ERROR("%s", msg);
	free(msg);
	msg=NULL;

	/* Return control to the setjmp point */
	longjmp(myerr->setjmp_buffer, 1);
}

void onion_init_destination(j_compress_ptr cinfo)
{
	onion_jpeg_data *d=(onion_jpeg_data*)cinfo->client_data;
	cinfo->dest->next_output_byte = (JOCTET *) &(d->res->buffer[d->res->buffer_pos]);
	cinfo->dest->free_in_buffer = sizeof(d->res->buffer)-d->res->buffer_pos;
}

boolean onion_empty_output_buffer(j_compress_ptr cinfo)
{
	onion_jpeg_data *d=(onion_jpeg_data*)cinfo->client_data;
	/*
	 * Note: It's not correct to substract free_in_buffer. This value
	 * should be zero, but this depends on the implementation of libjpeg.
	 * It's better to assume that all bytes of the buffer are used.
	d->res->buffer_pos = sizeof(d->res->buffer)-cinfo->dest->free_in_buffer;
	 */
	d->res->buffer_pos = sizeof(d->res->buffer);

	if(onion_response_flush(d->res)<0){
		// Flush failed.
		d->sent = 2;
		return FALSE;
	}

	cinfo->dest->next_output_byte = (JOCTET *) &d->res->buffer[d->res->buffer_pos];
	cinfo->dest->free_in_buffer = sizeof(d->res->buffer)-d->res->buffer_pos;
	return TRUE;
}

void onion_term_destination(j_compress_ptr cinfo)
{
	onion_jpeg_data *d=(onion_jpeg_data*)cinfo->client_data;
	d->res->buffer_pos = sizeof(d->res->buffer)-cinfo->dest->free_in_buffer;
	if(onion_response_flush(d->res)<0){
		// Flush failed.
		d->sent = 2;
	}
}

int onion_jpeg_response ( unsigned char * image,
		int image_num_color_channels,
		J_COLOR_SPACE image_color_space, /* See jpeglib.h for list of available color spaces. */
		int image_width,
		int image_height,
		int output_quality,
		onion_response *res
		)
{
	struct jpeg_compress_struct cinfo;
  struct onion_error_mgr jerr;
	/* More stuff */
	JSAMPROW row_pointer[1];      /* pointer to JSAMPLE row[s] */
	int row_stride;               /* physical row width in image buffer */

	onion_response_set_header(res, "Content-Type", "image/jpeg");
	if (onion_response_write_headers(res)==OR_SKIP_CONTENT) // Maybe it was HEAD.
		return OCS_PROCESSED;

	/* Step 1: allocate and initialize JPEG compression object */
	// 1.1 Setup used error routines
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = onion_error_exit;
	/* Establish the setjmp return context for onion_error_exit to use. */
	if (setjmp(jerr.setjmp_buffer)) {
		/* If we get here, the JPEG code has signaled an error.
		 * We need to clean up the JPEG object, and return.
		 */
		jpeg_destroy_compress(&cinfo);
		return 0;
	}

	jpeg_create_compress(&cinfo);

	/* Step 2: specify data destination */
	onion_jpeg_data ojd;
	ojd.res=res;
	ojd.sent=0;
	cinfo.client_data = (void *)&ojd;

	// Set function handler to operate directly on response buffers
	struct jpeg_destination_mgr dest;
	cinfo.dest = &dest;
	cinfo.dest->init_destination = &onion_init_destination;
	cinfo.dest->empty_output_buffer = &onion_empty_output_buffer;
	cinfo.dest->term_destination = &onion_term_destination;

	/* Step 3: set parameters for compression */

	/* First we supply a description of the input image.
	 * Four fields of the cinfo struct must be filled in:
	 */
	cinfo.image_width = image_width;      /* image width and height, in pixels */
	cinfo.image_height = image_height;
	cinfo.input_components = image_num_color_channels;           /* # of color components per pixel */
	cinfo.in_color_space = image_color_space;       /* colorspace of input image */
	/* Now use the library's routine to set default compression parameters.
	 * (You must set at least cinfo.in_color_space before calling this,
	 * since the defaults depend on the source color space.)
	 */

	jpeg_set_defaults(&cinfo);
	/* Now you can set any non-default parameters you wish to.
	 * Here we just illustrate the use of quality (quantization table) scaling:
	 */
	jpeg_set_quality(&cinfo, output_quality, TRUE /* limit to baseline-JPEG values */);

	// If header already sent, then there was an error.
	if (ojd.sent){
		return OCS_PROCESSED;
	}

	/* Step 4: Start compressor */

	/* TRUE ensures that we will write a complete interchange-JPEG file.
	 * Pass TRUE unless you are very sure of what you're doing.
	 */
	jpeg_start_compress(&cinfo, TRUE);

	/* Step 5: while (scan lines remain to be written) */
	/*           jpeg_write_scanlines(...); */

	/* Here we use the library's state variable cinfo.next_scanline as the
	 * loop counter, so that we don't have to keep track ourselves.
	 * To keep things simple, we pass one scanline per call; you can pass
	 * more if you wish, though.
	 */
	row_stride = image_width * image_num_color_channels; /* JSAMPLEs per row in image_buffer */

	while (cinfo.next_scanline < cinfo.image_height) {
		/* jpeg_write_scanlines expects an array of pointers to scanlines.
		 * Here the array is only one element long, but you could pass
		 * more than one scanline at a time if that's more convenient.
		 */
		row_pointer[0] = & image[cinfo.next_scanline * row_stride];
		(void) jpeg_write_scanlines(&cinfo, row_pointer, 1);

		// Check if response creation was aborted.
		if (ojd.sent){
			break;
		}
	}

	ojd.sent=1;

	/* Step 6: Finish compression */
	jpeg_finish_compress(&cinfo);

	/* Step 7: release JPEG compression object */
	jpeg_destroy_compress(&cinfo);

	/* That's it */
	return OCS_PROCESSED;
}

