/*
	Onion HTTP server library
	Copyright (C) 2010-2011 David Moreno Montero

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
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>

#include "onion.h"
#include "log.h"
#include "shortcuts.h"

/**
 * @short Shortcut for fast responses, like errors.
 * 
 * Prepares a fast response. You pass only the request, the text and the code, and it do the full response
 * object and sends the data.
 */
int onion_shortcut_response(const char* response, int code, onion_request* req)
{
	return onion_shortcut_response_extra_headers(response, code, req, NULL);
}


/**
 * @short Shortcut for fast responses, like errors, with extra headers
 * 
 * Prepares a fast response. You pass only the request, the text and the code, and it do the full response
 * object and sends the data.
 * 
 * On this version you also pass a NULL terminated list of headers, in key, value pairs.
 */
int onion_shortcut_response_extra_headers(const char* response, int code, onion_request* req, ... ){
	onion_response *res=onion_response_new(req);
	unsigned int l=strlen(response);
	const char *key, *value;
	
	onion_response_set_length(res,l);
	onion_response_set_code(res,code);
	
	va_list ap;
	va_start(ap, req);
	while ( (key=va_arg(ap, const char *)) ){
		value=va_arg(ap, const char *);
		if (key && value)
			onion_response_set_header(res, key, value);
		else
			break;
	}
	va_end(ap);

	onion_response_write_headers(res);
	
	onion_response_write(res,response,l);
	return onion_response_free(res);
}


/**
 * @short Shortcut to ease a redirect. 
 * 
 * It can be used directly as a handler, or be called from a handler.
 * 
 * The browser message is fixed; if need more flexibility, create your own redirector.
 */
int onion_shortcut_redirect(const char *newurl, onion_request *req){
	return onion_shortcut_response_extra_headers("<h1>302 - Moved</h1>", HTTP_REDIRECT, req,
																							 "Location", newurl, NULL );
}

/**
 * @short This shortcut returns the given file contents. 
 * 
 * It sets all the compilant headers (TODO), cache and so on.
 * 
 * This is the recomended way to send static files; it even can use sendfile Linux call 
 * if suitable (TODO).
 * 
 * It does no security checks, so caller must be security aware.
 */
int onion_shortcut_response_file(const char *filename, onion_request *request){
	int fd=open(filename,O_RDONLY);

	if (fd<0)
		return 0;

	struct stat st;
	if (stat(filename, &st)!=0){
		ONION_WARNING("File does not exist: %s",filename);
		return 0;
	}
	
	size_t size=st.st_size;
	size_t length=size;

	onion_response *res=onion_response_new(request);

	
	const char *range=onion_request_get_header(request, "Range");
	if (range && strncmp(range,"bytes=",6)==0){
		//ONION_DEBUG("Need just a range: %s",range);
		char tmp[1024];
		strncpy(tmp, range+6, 1024);
		char *start=tmp;
		char *end=tmp;
		while (*end!='-' && *end) end++;
		if (*end=='-'){
			*end='\0';
			end++;
			
			//ONION_DEBUG("Start %s, end %s",start,end);
			size_t ends, starts;
			if (*end)
				ends=atol(end);
			else
				ends=length;
			starts=atol(start);
			length=ends-starts;
			lseek(fd, starts, SEEK_SET);
			snprintf(tmp,sizeof(tmp),"%d-%d/%d",(unsigned int)starts, (unsigned int)ends-1, (unsigned int)length);
			onion_response_set_header(res, "Content-Range",tmp);
		}
	}
	
	onion_response_set_length(res, length);
	onion_response_write_headers(res);
	
	if ((onion_request_get_flags(request)&OR_HEAD) == OR_HEAD){ // Just head.
		length=0;
	}
	
	if (length){
		int r=0,w;
		size_t tr=0;
		char tmp[4046];
		if (length>sizeof(tmp)){
			size_t max=length-sizeof(tmp);
			while( tr<max ){
				r=read(fd,tmp,sizeof(tmp));
				tr+=r;
				if (r<0)
					break;
				w=onion_response_write(res, tmp, r);
				if (w!=r){
					ONION_ERROR("Wrote less than read: write %d, read %d. Quite probably closed connection.",w,r);
					break;
				}
			}
		}
		if (sizeof(tmp) >= (length-tr)){
			r=read(fd, tmp, length-tr);
			w=onion_response_write(res, tmp, r);
			if (w!=r){
				ONION_ERROR("Wrote less than read: write %d, read %d. Quite probably closed connection.",w,r);
			}
		}
	}
	close(fd);
	return onion_response_free(res);
}

