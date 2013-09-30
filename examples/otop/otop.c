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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/types.h>
#include <pwd.h>

#include <onion/onion.h>
#include <onion/handler.h>
#include <onion/response.h>
#include <onion/codecs.h>

#include <onion/handlers/exportlocal.h>
#include <onion/handlers/auth_pam.h>

onion_handler *otop_handler_new();
void otop_write_process_data(onion_response *res, int pid);
int otop_index(void *d, onion_request *req, onion_response *res);
int otop_handler(void *d, onion_request *req, onion_response *res);

/**
 * @short Exports a top like structure
 */
int main(int argc, char **argv){
	// Setup the server layout
	onion_url *withauth=onion_url_new();
	onion_url_add(withauth, "^ps/$", otop_handler);
	onion_url_add_handler(withauth, "^static/", onion_handler_export_local_new("."));
	onion_url_add(withauth, "", otop_index);
	
	onion_handler *otop=onion_handler_auth_pam("Onion Top", "login", onion_url_to_handler(withauth));
	
	// Create server and setup
	onion *onion=onion_new(O_ONE);
	onion_set_root_handler(onion, otop); // should be onion to ask for user.
	
	if (argc>1)
		onion_set_port(onion, argv[1]);
	else
		onion_set_port(onion, "8080");
	
	// Listen.
	int error=onion_listen(onion);
	if (error){
		perror("Cant create the server");
	}
	
	// done
	onion_free(onion);
	
	return !error;
}

/// The handler created here.
int otop_handler(void *d, onion_request *req, onion_response *res){
	DIR *dir=opendir("/proc/");
	if (!dir){
		onion_response_set_code(res,500);
		onion_response_write0(res,"Error reading proc directory");
		onion_response_write_headers(res);
		return 1;
	}
	onion_response_set_header(res, "Content-Type","text/html; charset: utf-8");
	onion_response_write_headers(res);
	
	onion_response_write0(res,"{");
	struct dirent *fi;
	int n=0;
	while ((fi = readdir(dir)) != NULL){
		char *p=NULL;
		int pid=strtol(fi->d_name, &p, 10);
		if (*p=='\0'){ // this means I converted it all to an integer.
			n++;
			//if (n>0 && n<10)
			otop_write_process_data(res, pid);
		}
	}
	closedir(dir);
	onion_response_write0(res,"\"-1\":{}}");
	
	return 1;
}


void otop_write_process_data(onion_response *res, int pid){
	char temp[1024];
	char temp2[1024];
	FILE *fd;
	onion_response_printf(res,"\"%d\":{",pid);
	
	sprintf(temp, "/proc/%d/status", pid);
	fd=fopen(temp,"r");
	if (!fd){ // Error.
		fprintf(stderr,"%s:%d Could not open %s proc status file.\n",__FILE__,__LINE__,temp);
		onion_response_write0(res,"},");
		return;
	}
	
	size_t l=sizeof(temp);
	char *t=temp;
	char prev=0;
	while (getline(&t,&l,fd)>0){
		char *p=temp;
		while(*p++!=':');
		p--;
		*p='\0';
		p++;
		char *val=p;
		while(*p!='\n' && *p!='\0') p++;
		*p='\0';
		if (strcmp(temp,"Name")==0){
			if (prev) onion_response_write(res,",",1);
			while(*val =='\t' || *val==' ') val++;
			onion_c_quote(val, temp2, sizeof(temp2));
			onion_response_printf(res,"\"%s\":%s",temp, temp2);
			prev=1;
		}
		else if (strcmp(temp,"PPid")==0){
			if (prev) onion_response_write(res,",",1);
			while(*val =='\t' || *val==' ') val++;
			onion_c_quote(val, temp2, sizeof(temp2));
			onion_response_printf(res,"\"%s\":%s",temp, temp2);
			prev=1;
		}
		else if (strcmp(temp,"Uid")==0){
			if (prev) onion_response_write(res,",",1);
			while(*val =='\t' || *val==' ') val++;
			struct passwd pw, *ok;
			getpwuid_r(atoi(val), &pw, temp2, sizeof(temp2), &ok);
			if (NULL==ok)
				continue;

			onion_response_printf(res,"\"%s\":\"%s\"",temp, pw.pw_name);
			prev=1;
		}
		else if (strcmp(temp,"VmRSS")==0){
			if (prev) onion_response_write(res,",",1);
			while(*val =='\t' || *val==' ') val++;
			onion_c_quote(val, temp2, sizeof(temp2));
			onion_response_printf(res,"\"%s\":%s",temp, temp2);
			prev=1;
		}
		else if (strcmp(temp,"VmData")==0){
			if (prev) onion_response_write(res,",",1);
			while(*val =='\t' || *val==' ') val++;
			onion_c_quote(val, temp2, sizeof(temp2));
			onion_response_printf(res,"\"%s\":%s",temp, temp2);
			prev=1;
		}
		else if (strcmp(temp,"VmLib")==0){
			if (prev) onion_response_write(res,",",1);
			while(*val =='\t' || *val==' ') val++;
			onion_c_quote(val, temp2, sizeof(temp2));
			onion_response_printf(res,"\"%s\":%s",temp, temp2);
			prev=1;
		}
	}
	fclose(fd);
	
	onion_response_write0(res,"}, ");
}

void opack_index_html(onion_response *res);

int otop_index(void *d, onion_request *req, onion_response *res){
	opack_index_html(res);
	return 1;
}

