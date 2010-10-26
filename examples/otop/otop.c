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
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>

#include <onion.h>
#include <onion_handler.h>
#include <onion_response.h>
#include <handlers/onion_handler_path.h>
#include <handlers/onion_handler_auth_pam.h>

onion_handler *otop_handler_new();
void otop_write_process_data(onion_response *res, int pid);
int otop_index(void *d, onion_request *req);
int otop_handler(void *d, onion_request *req);

/**
 * @short Exports a top like structure
 */
int main(int argc, char **argv){
	// Setup the server layout
	onion_handler *withauth=onion_handler_path("/ps/",
																				onion_handler_new((onion_handler_handler)otop_handler, NULL, NULL));
	onion_handler_add(withauth, onion_handler_new((onion_handler_handler)otop_index, NULL, NULL));
	onion_handler *otop=onion_handler_auth_pam("Onion Top", "login", withauth);
	
	// Create server and setup
	onion *onion=onion_new(O_ONE);
	onion_set_root_handler(onion, otop);
	
	if (argc>1)
		onion_set_port(onion, atoi(argv[1]));
	else
		onion_set_port(onion, 8080);
	
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
int otop_handler(void *d, onion_request *req){
	onion_response *res=onion_response_new(req);
	
	DIR *dir=opendir("/proc/");
	if (!dir){
		onion_response_set_code(res,500);
		onion_response_write0(res,"Error reading proc directory");
		onion_response_write_headers(res);
		return 1;
	}
	onion_response_write_headers(res);
	
	struct dirent *fi;
	while ((fi = readdir(dir)) != NULL){
		char *p=NULL;
		int pid=strtol(fi->d_name, &p, 10);
		if (*p=='\0'){ // this means I converted it all to an integer.
			otop_write_process_data(res, pid);
		}
	}
	closedir(dir);
	
	onion_response_free(res);
	
	return 1;
}


void otop_write_process_data(onion_response *res, int pid){
	char temp[1024];
	FILE *fd;
	sprintf(temp, "%d ", pid);
	onion_response_write0(res, temp);

	// read cmdline. Only first arg.
	sprintf(temp, "/proc/%d/cmdline", pid);
	fd=fopen(temp,"r");
	if (!fd){ // Error.
		fprintf(stderr,"%s:%d Could not open %s proc file.\n",__FILE__,__LINE__,temp);
		return;
	}
	fscanf(fd,"%s",temp);
	onion_response_write0(res, temp);
	
	fclose(fd);
	
	onion_response_write(res, "\n", 1);
}

void opack_index_html(onion_response *res);

int otop_index(void *d, onion_request *req){
	onion_response *res=onion_response_new(req);
	
	onion_response_write_headers(res);
	
	opack_index_html(res);
	
	return 1;
}

