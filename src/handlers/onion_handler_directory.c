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
#include <malloc.h>
#include <unistd.h>
#include <regex.h>
#include <stdio.h>
#include <fcntl.h>

#include <onion_handler.h>
#include <onion_response.h>

#include "onion_handler_directory.h"

struct onion_handler_directory_data_t{
	regex_t path;
	char *localpath;
};

typedef struct onion_handler_directory_data_t onion_handler_directory_data;

int onion_handler_directory_handler(onion_handler *handler, onion_request *request){
	onion_handler_directory_data *d=handler->priv_data;
	regmatch_t match[1];
	const char *path=onion_request_get_path(request);
	
	if (regexec(&d->path, path, 1, match, 0)!=0)
		return 0;
	
	onion_request_advance_path(request, match[0].rm_eo);

	char tmp[1024];
	sprintf(tmp,"%s/%s",d->localpath,onion_request_get_path(request));


	int fd=open(tmp,O_RDONLY);
	if (fd<0){ // cant open. Continue on next. Quite probably a custom error.
		return 0;
	}
	onion_response *res=onion_response_new(request);
	onion_response_write_headers(res);
	int r;
	while( (r=read(fd,tmp,sizeof(tmp))) !=0 ){
		onion_response_write(res, tmp, r);
	}
	close(fd);
	onion_response_free(res);
	return 1;
}


void onion_handler_directory_delete(void *data){
	onion_handler_directory_data *d=data;
	regfree(&d->path);
	free(d->localpath);
	free(d);
}

/**
 * @short Creates an path handler. If the path matches the regex, it reomves that from the regexp and goes to the inside_level.
 *
 * If on the inside level nobody answers, it just returns NULL, so ->next can answer.
 */
onion_handler *onion_handler_directory(const char *path, const char *localpath){
	onion_handler *ret;
	ret=malloc(sizeof(onion_handler));
	memset(ret,0,sizeof(onion_handler));
	
	onion_handler_directory_data *priv_data=malloc(sizeof(onion_handler_directory_data));

	priv_data->localpath=strdup(localpath);
	
	// Path is a little bit more complicated, its an regexp.
	int err;
	if (path)
		err=regcomp(&priv_data->path, path, REG_EXTENDED);
	else
		err=regcomp(&priv_data->path, "", REG_EXTENDED); // empty regexp, always true. should be fast enough. 
	if (err){
		char buffer[1024];
		regerror(err, &priv_data->path, buffer, sizeof(buffer));
		fprintf(stderr, "%s:%d Error analyzing regular expression '%s': %s.\n", __FILE__,__LINE__, path, buffer);
		onion_handler_directory_delete(priv_data);
		return NULL;
	}
	
	ret->handler=onion_handler_directory_handler;
	ret->priv_data=priv_data;
	ret->priv_data_delete=onion_handler_directory_delete;
	
	return ret;
}

