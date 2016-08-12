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

	You should have received a copy of both licenses, if not see
	<http://www.gnu.org/licenses/> and
	<http://www.apache.org/licenses/LICENSE-2.0>.
	*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "mime.h"
#include "dict.h"
#include "log.h"
#include "utils.h"

/// @defgroup mime MIME. mime functionctionalities

/// Dict with all extension to mime type. This will never be freed.
static onion_dict *onion_mime_dict=NULL;

static void onion_mime_fill();

/**
 * @short Sets a user set dict as mime dict
 * @ingroup mime
 *
 * This dict maps "extension" -> "mimetype".
 *
 * At onion_server_free it is freed, as if this function is called again.
 */
void onion_mime_set(onion_dict *d){
	if (onion_mime_dict)
		onion_dict_free(onion_mime_dict);
	onion_mime_dict=d;
}


/**
 * @short Given a filename or extensiton, it returns the proper mime type.
 * @ingroup mime
 *
 * If onion_mime_set was called before, it is used, else it reads mime types /etc/mime.types.
 *
 * Full mime catalog, from /etc/mime.types, takes about 36kb on ubuntu 10.10, may depend on
 * how many mime types are known.
 *
 * If none is found, returns text/plain.
 */
const char *onion_mime_get(const char *filename){
	if (!onion_mime_dict)
		onion_mime_fill();

	const char *extension=filename;
	int l=strlen(filename);
	int i;
	for (i=l;i--;){
		if (extension[i]=='.'){
			extension=&extension[i+1];
			break;
		}
	}

	//ONION_DEBUG("I know %d mime types", onion_dict_count(onion_mime_dict));

	const char *r=onion_dict_get(onion_mime_dict, extension);
	if (r)
		return r;
	//ONION_DEBUG("Mime type for extension '%s' %s",extension, r);
	return "text/plain";
}

/**
 * @short Reads the /etc/mime.types file
 * @ingroup mime
 *
 *
 */
static void onion_mime_fill(){
	onion_mime_set(NULL);
	onion_mime_dict=onion_dict_new();
	//ONION_DEBUG("Filling mime types");

	FILE *fd=fopen("/etc/mime.types", "rt");
	if (!fd){
		ONION_WARNING("Could not read MIME types (etc/mime.types), returned mime types may be incorrect. Adding minimal set.");
		onion_dict_add(onion_mime_dict, "html", "text/html",0);
		onion_dict_add(onion_mime_dict, "htm", "text/html",0);
		onion_dict_add(onion_mime_dict, "js", "application/javascript",0);
		onion_dict_add(onion_mime_dict, "css", "text/css",0);
		onion_dict_add(onion_mime_dict, "png", "image/png",0);
		onion_dict_add(onion_mime_dict, "jpg", "image/jpeg",0);
		return;
	}
	char mimetype[128];
	char extension[8];
	int mode=0; // 0 mimetype, 1 extension
	int i=0;
	int c;

	while ( (c=getc(fd)) >= 0){
		if (c=='#'){
			while ( (c=getc(fd)) >= 0 && c!='\n');
		}
		if (c=='\n'){
			if (mode==1 && i!=0){
				extension[i]=0;
				onion_dict_add(onion_mime_dict, extension, mimetype, OD_DUP_ALL);
				//ONION_DEBUG("Add mimetype '%s' (%s).", extension, mimetype);
			}
			mode=0;
			i=0;
		}
		else{
			if (is_space(c)){
				if (mode==0){
					mimetype[i]='\0';
					mode=1;
					i=0;
				}
				else if (i!=0){
					extension[i]='\0';
					i=0;
					onion_dict_add(onion_mime_dict, extension, mimetype, OD_DUP_ALL);
					//ONION_DEBUG("Add mimetype '%s' (%s)", extension, mimetype);
				}
			}
			else{
				if (mode==0){
					if (i>=sizeof(mimetype)-1){
						while ( (c=getc(fd)) >= 0 && c!='\n');
					}
					else
						mimetype[i++]=c;
				}
				else{
					if (i>=sizeof(extension)-1){
						while ( (c=getc(fd)) >= 0 && c!='\n');
						extension[i]='\0';
						i=0;
						mode=0;
						onion_dict_add(onion_mime_dict, extension, mimetype,  OD_DUP_ALL);
						//ONION_DEBUG("Add mimetype '%s' (%s)..", extension, mimetype);
					}
					else
						extension[i++]=c;
				}
			}
		}
	}
	fclose(fd);

	ONION_DEBUG("I know %d mime types", onion_dict_count(onion_mime_dict));
}

/**
 * @short Allow to update mime types.
 * @ingroup mime
 *
 * User can add new mime types, or remove (if mimetype == NULL).
 */
void onion_mime_update(const char *extension, const char *mimetype){
	if (!onion_mime_dict)
		onion_mime_fill();

	if (mimetype)
		onion_dict_add(onion_mime_dict, extension, mimetype,  OD_DUP_ALL);
	else
		onion_dict_remove(onion_mime_dict, extension);
}
