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

#include <string.h>
#include <stdio.h>

#include <onion/log.h>
#include <onion/onion.h>

#ifdef PNG_ENABLED
#include <png.h>
#include <onion/extras/png.h>
#endif

#ifdef JPEG_ENABLED
#include <stdlib.h>
#include <jpeglib.h>
#include <onion/extras/jpeg.h>
#endif

#define MAX(X,Y) (((X)>(Y))?(X):(Y))
#define MIN(X,Y) (((X)<(Y))?(X):(Y))

/// Basic complex class, to ease the fractal calculation
class Complex{
public:
	Complex(){ r=i=0.0; }
	Complex(double _r, double _i){ r=_r; i=_i; }
	Complex(const Complex &c){ r=c.r; i=c.i; }
	Complex operator*(const Complex &B){ return Complex(r*B.r - i*B.i, r*B.i + i*B.r); }
	Complex operator+(const Complex &B){ return Complex(r+B.r, i+B.i); }
	double lenlen(){ // the ^2 of the length. Also known as why sqrt if I dont care
		return r*r + i*i;
	}
private:
	double r;
	double i;
};

#ifdef PNG_ENABLED
/**
 * @short Calculates the given mandelbrot, and returns the PNG image
 * 
 * It can receive several query parameters, but all are optional:
 * 
 * * X, Y -- Top left complex area position
 * * W, H -- Width and Height of the complex area to show
 * * C -- 0 grayscale image , 1 - coloured image
 * * width, height -- Image size
 */
int mandelbrotPNG(void *p, onion_request *req, onion_response *res){
	int width=atoi(onion_request_get_queryd(req,"width","256"));
	int height=atoi(onion_request_get_queryd(req,"height","256"));
	int color=atoi(onion_request_get_queryd(req,"C","0"));

	if (width<=0 || height<=0)
		return OCS_INTERNAL_ERROR;

	int channels = 1;
	if( color == 1)
		channels = 4;

	unsigned char *image=new unsigned char[channels*width*height];
  int    i,j,n;

	double left=atof(onion_request_get_queryd(req,"X","-2"));
	double top=atof(onion_request_get_queryd(req,"Y","-2"));
	double right=left + atof(onion_request_get_queryd(req,"W","4"));
	double bottom=top + atof(onion_request_get_queryd(req,"H","4"));

  double stepX = (right-left)/width;
  double stepY = (bottom-top)/height;
	int steps=100;
	unsigned char *imagep=image;

  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
			Complex z;
			Complex c(stepX*j + left, stepY*i + top);
      for (n=0;n<steps;n++){
				z=z*z + c;
				if (z.lenlen() > steps)
					break;
			}
			char P;
			if (n >= steps) P=255;
			else P=(n*256)/steps;

			if( channels == 4 ){
				*imagep++=P;
				*imagep++=2*P+30;
				*imagep++=255-P*3;
				*imagep++=255;
			}else{
				*imagep++=P;
			}
    }
  }

	onion_png_response(image, channels, width, height, res);



	delete[] image;
	return OCS_PROCESSED;
}
#endif

#ifdef JPEG_ENABLED
int mandelbrotJPEG(void *p, onion_request *req, onion_response *res){
	int width=atoi(onion_request_get_queryd(req,"width","256"));
	int height=atoi(onion_request_get_queryd(req,"height","256"));
	int color=atoi(onion_request_get_queryd(req,"C","0"));
	int jpgQuality=MAX(0,MIN(atoi(onion_request_get_queryd(req,"Q","100")),100));

	if (width<=0 || height<=0)
		return OCS_INTERNAL_ERROR;

	int channels = 1;
	if( color == 1)
		channels = 3;

	unsigned char *image=new unsigned char[channels*width*height];
  int    i,j,n;

	double left=atof(onion_request_get_queryd(req,"X","-2"));
	double top=atof(onion_request_get_queryd(req,"Y","-2"));
	double right=left + atof(onion_request_get_queryd(req,"W","4"));
	double bottom=top + atof(onion_request_get_queryd(req,"H","4"));

  double stepX = (right-left)/width;
  double stepY = (bottom-top)/height;
	int steps=100;
	unsigned char *imagep=image;

  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
			Complex z;
			Complex c(stepX*j + left, stepY*i + top);
      for (n=0;n<steps;n++){
				z=z*z + c;
				if (z.lenlen() > steps)
					break;
			}
			char P;
			if (n >= steps) P=255;
			else P=(n*256)/steps;

			if( channels == 3 ){
				*imagep++=P;
				*imagep++=2*P+30;
				*imagep++=255-P*3;
			}else{
				*imagep++=P;
			}
    }
  }

	onion_jpeg_response(image, channels,
			(channels==1?JCS_GRAYSCALE:JCS_RGB)
			, width, height, jpgQuality, res);

	delete[] image;
	return OCS_PROCESSED;
}
#endif

// This has to be extern, as we are compiling C++
extern "C"{
int mandel_html_template(void *, onion_request *req, onion_response *res);
}

int main(int argc, char **argv){
	onion *o=onion_new(O_THREADED);

	onion_url *url=onion_root_url(o);

	onion_set_hostname(o, "0.0.0.0"); // Force ipv4.
#ifdef PNG_ENABLED
	onion_url_add(url, "mandel.png", (void*)mandelbrotPNG);
#endif
#ifdef JPEG_ENABLED
	onion_url_add(url, "mandel.jpg", (void*)mandelbrotJPEG);
#endif
	onion_url_add(url, "", (void*)mandel_html_template);

	onion_listen(o);
	onion_free(o);
	return 0;
}
