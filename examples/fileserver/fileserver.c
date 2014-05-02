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
#include <onion/shortcuts.h>

#include <onion/handlers/exportlocal.h>
#include <onion/handlers/path.h>
#include <onion/handlers/static.h>
#include <onion/handlers/auth_pam.h>

onion *o=NULL;

void free_onion(int unused){
	ONION_INFO("Closing connections");
	onion_free(o);
	exit(0);
}

typedef struct{
	const char *abspath;
}upload_file_data;


onion_connection_status upload_file(upload_file_data *data, onion_request *req, onion_response *res){
	if (onion_request_get_flags(req)&OR_POST){
		const char *name=onion_request_get_post(req,"file");
		const char *filename=onion_request_get_file(req,"file");
		
		if (name && filename){
			char finalname[1024];
			snprintf(finalname,sizeof(finalname),"%s/%s",data->abspath,name);
			ONION_DEBUG("Copying from %s to %s",filename,finalname);

			onion_shortcut_rename(filename, finalname);
		}
	}
	return 0; // I just ignore the request, but process over the FILE data
}

/// Footer that allows to upload files.
void upload_file_footer(onion_response *res, const char *dirname){
	onion_response_write0(res, "<p><table><tr><th>Upload a file: <form method=\"POST\" enctype=\"multipart/form-data\">"
														 "<input type=\"file\" name=\"file\"><input type=\"submit\"><form></th></tr></table>");
	onion_response_write0(res,"<h2>Onion minimal fileserver. (C) 2010 <a href=\"http://www.coralbits.com\">CoralBits</a>. "
														"Under <a href=\"http://www.gnu.org/licenses/agpl-3.0.html\">AGPL 3.0.</a> License.</h2>\n");
}

int show_help(){
	printf("Onion basic fileserver. (C) 2010 Coralbits S.L.\n\n"
				"Usage: fileserver [options] [directory to serve]\n\n"
				"Options:\n"
				"  --pem pemfile   Uses that certificate and key file. Both must be on the same file. Default is at current dir cert.pem.\n"
				"  --pam pamname   Uses that pam name to allow access. Default login.\n"
				"  --port N\n" 
				"   -p N           Listens at given port. Default 8080\n"
				"  --listen ADDRESS\n"
				"   -l ADDRESS     Listen to that address. It can be a IPv4 or IPv6 IP, or a local host name. Default: 0.0.0.0\n"
				"  --help\n"
				"   -h             Shows this help\n"
				"\n"
				"fileserver serves an HTML with easy access to current files, and allows to upload new files.\n"
				"It have SSL encrypting and PAM authentication.\n"
				"\n");
	return 0;
}


int main(int argc, char **argv){
	char *port="8080";
	char *hostname="::";
	const char *dirname=".";
	const char *certfile="cert.pem";
	const char *pamname="login";
	int i;
	for (i=1;i<argc;i++){
		if ((strcmp(argv[i],"--port")==0) || (strcmp(argv[i],"-p")==0)){
			port=argv[++i];
			ONION_INFO("Listening at port %s",port);
		}
		if ((strcmp(argv[i],"--listen")==0) || (strcmp(argv[i],"-l")==0)){
			hostname=argv[++i];
			ONION_INFO("Listening at hostname %s",hostname);
		}
		else if (strcmp(argv[i],"--pem")==0){
			if (argc<i+1)
				return show_help();
			certfile=argv[++i];
			ONION_INFO("Certificate file set to %s",certfile);
		}
		else if (strcmp(argv[i],"--pam")==0){
			if (argc<i+1)
				return show_help();
			pamname=argv[++i];
			ONION_INFO("Pam name is now %s",pamname);
		}
		else if (strcmp(argv[i],"--help")==0 || strcmp(argv[i],"-h")==0){
			return show_help();
		}
		else
			dirname=argv[i];
	}
	
	upload_file_data data={
		dirname
	};

	onion_handler *root=onion_handler_new((void*)upload_file,(void*)&data,NULL);
	onion_handler *dir=onion_handler_export_local_new(argc==2 ? argv[1] : ".");
	onion_handler_export_local_set_footer(dir, upload_file_footer);
	onion_handler_add(dir, onion_handler_static("<h1>404 - File not found.</h1>", 404) );
	onion_handler_add(root,dir);
	onion_handler *pam=onion_handler_auth_pam("Onion Fileserver", pamname, root);

	
	o=onion_new(O_THREADED);
	onion_set_root_handler(o, pam);
	onion_set_certificate(o, O_SSL_CERTIFICATE_KEY, certfile, certfile);
	
	
	onion_set_port(o, port);
	onion_set_hostname(o, hostname);
	
	signal(SIGINT, free_onion);
	int error=onion_listen(o);
	if (error){
		perror("Cant create the server");
	}
	
	onion_free(o);
	 
	return 0;
}
