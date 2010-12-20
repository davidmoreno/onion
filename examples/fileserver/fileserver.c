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
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

#include <onion/onion.h>
#include <onion/request.h>
#include <onion/response.h>
#include <onion/handler.h>
#include <onion/log.h>

#include <onion/handlers/directory.h>
#include <onion/handlers/static.h>
#include <onion/handlers/path.h>

onion *o=NULL;

void free_onion(){
	ONION_INFO("Closing connections");
	onion_free(o);
	exit(0);
}

typedef struct{
	const char *abspath;
}upload_file_data;

onion_connection_status upload_file(upload_file_data *data, onion_request *req){
	if (onion_request_get_flags(req)&OR_POST){
		const char *name=onion_request_get_post(req,"file");
		const char *filename=onion_request_get_file(req,"file");
		
		if (name && filename){
			char finalname[1024];
			snprintf(finalname,sizeof(finalname),"%s/%s",data->abspath,name);
			ONION_DEBUG("Copying from %s to %s",filename,finalname);

			int src=open(filename,O_RDONLY);
			int dst=open(finalname, O_WRONLY|O_CREAT, 0666);
			if (!src || !dst){
				ONION_ERROR("Could not open src or dst file (%d %d)",src,dst);
				return OCS_INTERNAL_ERROR;
			}
			ssize_t r,w;
			char buffer[1024*4];
			while ( (r=read(src,buffer,sizeof(buffer))) > 0){
				w=write(dst,buffer,r);
				if (w!=r){
					ONION_ERROR("Error writing file");
					break;
				}
			}
			close(src);
			close(dst);
		}
	}
	onion_response *res=onion_response_new(req);
	onion_response_write_headers(res);
	onion_response_write0(res, "<html><body><form method=\"POST\"  enctype=\"multipart/form-data\">"
														 "<input type=\"file\" name=\"file\"><input type=\"submit\"><form></body></html>");
	return onion_response_free(res);
}


int main(int argc, char **argv){
	int port=8080;
	const char *dirname=".";
	int i;
	for (i=1;i<argc;i++){
		if (strcmp(argv[i],"-p")==0){
			port=atoi(argv[++i]);
		}
		else
			dirname=argv[i];
	}
	
	upload_file_data data={
		dirname
	};
	
	onion_handler *root=onion_handler_path("upload", onion_handler_new((void*)upload_file,(void*)&data,NULL));
																				 
	onion_handler *dir=onion_handler_directory(argc==2 ? argv[1] : ".");
	onion_handler_add(dir, onion_handler_static(NULL,"<h1>404 - File not found.</h1>", 404) );
	onion_handler_add(root,dir);
	
	o=onion_new(O_THREADED|O_DETACH_LISTEN);
	onion_set_root_handler(o, root);
	
	
	onion_set_port(o, port);
	
	signal(SIGINT, free_onion);
	int error=onion_listen(o);
	if (error){
		perror("Cant create the server");
	}
	fprintf(stderr,"Detached. Printing a message every 10 seconds.\n");
	i=0;
	while(1){
		i++;
		ONION_INFO("Still alive. Count %d.",i);
		sleep(10);
	}
	
	onion_free(o);
	
	return 0;
}
