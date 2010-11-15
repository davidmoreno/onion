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
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>

#include <onion_handler.h>
#include <onion_response.h>
#include <onion_codecs.h>

#include "onion_handler_directory.h"

struct onion_handler_directory_data_t{
	char *localpath;
};

typedef struct onion_handler_directory_data_t onion_handler_directory_data;

int onion_handler_directory_handler_directory(const char *realp, const char *showpath, onion_request *req);
int onion_handler_directory_handler_file(const char *realp, struct stat *reals, onion_request *request);

int onion_handler_directory_handler(onion_handler_directory_data *d, onion_request *request){
	char tmp[PATH_MAX];
	char realp[PATH_MAX];
	sprintf(tmp,"%s/%s",d->localpath,onion_request_get_path(request));

	realpath(tmp, realp);
	if (strncmp(realp, d->localpath, strlen(d->localpath))!=0) // out of secured dir.
		return 0;

	//fprintf(stderr,"%s\n",realp);
	struct stat reals;
	int ok=stat(realp,&reals);
	if (ok<0) // Cant open for even stat
		return 0;

	if (S_ISDIR(reals.st_mode)){
		return onion_handler_directory_handler_directory(realp, onion_request_get_path(request), request);
	}
	else if (S_ISREG(reals.st_mode)){
		return onion_handler_directory_handler_file(realp, &reals, request);
	}
	return 0;
}

/**
 * @short Handler a single file.
 */
int onion_handler_directory_handler_file(const char *realp, struct stat *reals, onion_request *request){
		int fd=open(realp,O_RDONLY);

		onion_response *res=onion_response_new(request);
		onion_response_set_length(res, reals->st_size);
		onion_response_write_headers(res);
			
		int r;
		char tmp[4046];
		while( (r=read(fd,tmp,sizeof(tmp))) > 0 ){
			onion_response_write(res, tmp, r);
		}
		close(fd);
		return onion_response_free(res);
}

/**
 * @short Returns the directory listing
 */
int onion_handler_directory_handler_directory(const char *realp, const char *showpath, onion_request *req){
	DIR *dir=opendir(realp);
	if (!dir) // Continue on next. Quite probably a custom error.
		return 0;
	onion_response *res=onion_response_new(req);
	onion_response_set_header(res, "Content-Type", "text/html; charset=utf-8");
	onion_response_write_headers(res);
	
	onion_response_write0(res,"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
														"<html>\n"
														" <head><meta content=\"text/html; charset=UTF-8\" http-equiv=\"content-type\"/>\n");
	onion_response_printf(res,"<title>Directory listing of %s</title>\n",showpath);
	onion_response_write0(res,"</head>\n" 
														" <body>\n"
														"<style>body{ background: #fefefe; font-family: sans-serif; margin-left: 5%; margin-right: 5%; }"
														" table{ background: white; width: 100%; border: 1px solid #aaa; border-radius: 5px; -moz-border-radius: 5px; } "
														" th{	background: #eee; } tbody tr:hover td{ background: yellow; } tr.dir td{ background: #D4F0FF; }"
														" table a{ display: block; } th{ cursor: pointer} h1,h2{ color: black; text-align: center; } "
														" a{ color: red; text-decoration: none; }</style>\n"
														"<script src=\"http://code.jquery.com/jquery-1.4.3.min.js\"></script>\n"
														"<script src=\"http://tablesorter.com/jquery.tablesorter.min.js\"></script>\n"
														"<script>$(document).ready(function(){ $('table').tablesorter({sortList: [ [0,0] ] }); });</script>\n");
	onion_response_printf(res,"<h1>Listing of directory %s</h1>\n",showpath);
	
	if (showpath[1]!='\0') // It will be 0, when showpath is "/"
		onion_response_write0(res,"<h2><a href=\"..\">Go up..</a></h2>\n");
	
	onion_response_write0(res,"<table>\n<thead><tr><th>Filename</th><th>Size</th><th>Owner</th></tr></thead>\n<tbody>\n");
	
	struct dirent *fi;
	struct stat    st;
	char temp[1024];
	struct passwd  *pwd;
	while ( (fi=readdir(dir)) != NULL ){
		if (fi->d_name[0]=='.')
			continue;
		snprintf(temp,sizeof(temp),"%s/%s",realp,fi->d_name);
		stat(temp, &st);
		pwd=getpwuid(st.st_uid);
		
		char *quotedName=onion_quote_new(fi->d_name);
		
		if (S_ISDIR(st.st_mode))
			onion_response_printf(res, "<tr class=\"dir\"><td><a href=\"%s/\">%s</a></td><td>%d</td><td>%s</td></tr>\n", quotedName, fi->d_name, st.st_size, pwd->pw_name);
		else
			onion_response_printf(res, "<tr class=\"file\"><td><a href=\"%s\">%s</a></td><td>%d</td><td>%s</td></tr>\n", quotedName, fi->d_name, st.st_size, pwd->pw_name);
		free(quotedName);
	}

	onion_response_write0(res,"</tbody>\n</table>\n</body>\n");
	onion_response_write0(res,"<h2>Onion directory list. (C) 2010 <a href=\"http://www.coralbits.com\">CoralBits</a>. "
														"Under <a href=\"http://www.gnu.org/licenses/agpl-3.0.html\">AGPL 3.0.</a> License.</h2>\n</html>");

	int r=onion_response_free(res);
	
	closedir(dir);
	
	return r;
}


void onion_handler_directory_delete(void *data){
	onion_handler_directory_data *d=data;
	free(d->localpath);
	free(d);
}

/**
 * @short Creates an path handler. If the path matches the regex, it reomves that from the regexp and goes to the inside_level.
 *
 * If on the inside level nobody answers, it just returns NULL, so ->next can answer.
 */
onion_handler *onion_handler_directory(const char *localpath){
	onion_handler_directory_data *priv_data=malloc(sizeof(onion_handler_directory_data));
	char realp[PATH_MAX];
	realpath(localpath,realp);
	priv_data->localpath=strdup(realp);
	
	onion_handler *ret=onion_handler_new((onion_handler_handler)onion_handler_directory_handler,
																			 priv_data,(onion_handler_private_data_free) onion_handler_directory_delete);
	return ret;
}

