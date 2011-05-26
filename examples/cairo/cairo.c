/*
	Onion HTTP server library
	Copyright (C) 2011 David Moreno Montero

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

#include <math.h>

#include <cairo/cairo.h>

#include <onion/log.h>
#include <onion/onion.h>
#include <onion/mime.h>
#include <onion/extras/png.h>


onion_connection_status cairo_img(void *v, onion_request *req, onion_response *res){
	cairo_surface_t *surface;
	cairo_t *cr;
	
	int width=800;
	int height=600;
	float m=10.5;
	
	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
	cr = cairo_create (surface);
	
	// Draw the grid
	cairo_set_source_rgb (cr, 0.97, 0.98, 0.95);
	cairo_paint(cr);
	
	cairo_set_line_width(cr, 2.0);
	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_move_to(cr, m, m);
	cairo_line_to(cr, m, height-5);
	cairo_stroke(cr);
	
	cairo_move_to(cr, 5, height-m);
	cairo_line_to(cr, width-m, height-m);
	cairo_stroke(cr);

	// The bakground grid
	cairo_set_line_width(cr, 1.0);
	float i;
	for (i=30+m;i<width;i+=30){
		cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
		cairo_move_to(cr, 5+m, height-i);
		cairo_line_to(cr, width-m, height-i);
		cairo_stroke(cr);

		cairo_move_to(cr, i, m);
		cairo_line_to(cr, i, height-5);
		cairo_stroke(cr);
		
		cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
		cairo_move_to(cr, 5, height-i);
		cairo_line_to(cr, 5+m, height-i);
		cairo_stroke(cr);
		
		cairo_move_to(cr, i, height-5-m);
		cairo_line_to(cr, i, height-5);
		cairo_stroke(cr);
	}
		
	cairo_set_line_width(cr, 2.0);
	cairo_set_source_rgb(cr, 1.0, 0, 0);
	float h2=height/2;
	float scale=h2*0.7;
	for (i=m;i<width-m;i+=1.0){
		float v=i/10.0;
		float s=sin(sin(v)*2);
		cairo_line_to(cr, i, s*scale + h2);
	}
	cairo_stroke(cr);
	
	unsigned char *image=cairo_image_surface_get_data(surface);
	int ok=onion_png_response(image, -4, width, height, res);
	
	cairo_surface_destroy(surface);

	return ok;
}

int main(int argc, char **argv){
	onion *o=onion_new(O_THREADED);
	
	onion_url *url=onion_root_url(o);
	onion_url_add(url,"", cairo_img);
	
	onion_listen(o);
	
	return 0;
}

