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

#include <onion_ssl.h>
#include <onion_handler.h>
#include <onion_handler_path.h>
#include <onion_handler_static.h>
#include <onion_handler_auth_pam.h>


#include "oterm_handler.h"
#include <onion_handler_directory.h>

onion_ssl *o=NULL;

/// Prints some help to stderr
void show_help(){
	fprintf(stderr," oterm - Linux terminal over HTTP\n\n"
								 "It uses HTTP(S)+HTML+CSS+JavaScript to show a remote terminal inside a browser.\n\n"
								 "Options:\n"
								 "   -p|--port <port_number>      -- Set the port number to use. Default 8080\n"
								 "   -c|--cert <certificate file> -- Set the SSL certificate file. Default: /etc/pki/tls/certs/pound.pem\n"
								 "   -k|--key  <key file>         -- Set the SSL key file. Default: /etc/pki/tls/certs/pound.key\n"
								 "\n");
}

void free_onion(){
	fprintf(stderr,"\nClosing connections.\n");
	onion_ssl_free(o);
	exit(0);
}

int main(int argc, char **argv){
	int port=8080;
	const char *certificatefile="/etc/pki/tls/certs/pound.pem";
	const char *keyfile="/etc/pki/tls/certs/pound.key";
	
	int i;
	for (i=1;i<argc;i++){
		if (strcmp(argv[i],"--help")==0){
			show_help();
			exit(0);
		}
		else if(strcmp(argv[i],"-p")==0 || strcmp(argv[i],"--port")==0){
			if (i+1>argc){
				fprintf(stderr, "Need to set the port number.\n");
				show_help();
				exit(1);
			}
			port=atoi(argv[++i]);
			fprintf(stderr, "Using port %d\n",port);
		}
		else if(strcmp(argv[i],"-c")==0 || strcmp(argv[i],"--cert")==0){
			if (i+1>argc){
				fprintf(stderr, "Need to set the certificate filename\n");
				show_help();
				exit(1);
			}
			certificatefile=argv[++i];
			fprintf(stderr, "Using port %d\n",port);
		}
		else if(strcmp(argv[i],"-k")==0 || strcmp(argv[i],"--key")==0){
			if (i+1>argc){
				fprintf(stderr, "Need to set the certificate key filename.\n");
				show_help();
				exit(1);
			}
			keyfile=argv[++i];
			fprintf(stderr, "Using port %d\n",port);
		}
	}
	

#ifdef __DEBUG__
	onion_handler *dir=onion_handler_directory(".");
#else
	onion_handler *dir=onion_handler_path("^/$",oterm_handler_index());
	onion_handler_add(dir, onion_handler_path("^/oterm.js$",oterm_handler_oterm()));
#endif

	onion_handler_add(dir, onion_handler_path("^/term$",oterm_handler_data()));
	onion_handler_add(dir, onion_handler_static(NULL,"<h1>404 - File not found.</h1>", 404) );

	onion_handler *oterm=onion_handler_auth_pam("Onion Terminal", "login", dir);


	
	o=onion_ssl_new(O_ONE);
	onion_ssl_set_root_handler(o, oterm);
	int error=onion_ssl_use_certificate(o, O_SSL_CERTIFICATE_KEY, certificatefile, keyfile);
	if (error){
		fprintf(stderr, "Cant set certificate and key files (%s, %s)\n",certificatefile, keyfile);
		show_help();
		exit(1);
	}
	
	onion_ssl_set_port(o, port);
	
	signal(SIGINT, free_onion);
	signal(SIGPIPE, SIG_IGN);
	fprintf(stderr, "Listening at %d\n",port);
	error=onion_ssl_listen(o);
	if (error){
		perror("Cant create the server");
	}
	
	onion_ssl_free(o);
	
	return 0;
}
