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

#include <onion/onion.h>
#include <onion/handler.h>

#include <onion/handlers/path.h>
#include <onion/handlers/static.h>
#include <onion/handlers/auth_pam.h>
#include <onion/handlers/opack.h>

#ifdef __DEBUG__
#include <onion/handlers/directory.h>
#endif 

#include "oterm_handler.h"


void opack_index_html(onion_response *res);
void opack_jquery_1_4_3_min_js(onion_response *res);
extern unsigned int opack_index_html_length;
extern unsigned int opack_jquery_1_4_3_min_js_length;


onion *o=NULL;

/// Prints some help to stderr
void show_help(){
	fprintf(stderr," oterm - Linux terminal over HTTP\n\n"
								 "It uses HTTP(S)+HTML+CSS+JavaScript to show a remote terminal inside a browser.\n\n"
								 "Options:\n"
								 "   -p|--port <port_number>      -- Set the port number to use. Default 8080\n"
								 "   -c|--cert <certificate file> -- Set the SSL certificate file. Default: /etc/pki/tls/certs/pound.pem\n"
								 "   -k|--key  <key file>         -- Set the SSL key file. Default: /etc/pki/tls/certs/pound.key\n"
								 "   --no-ssl                     -- Do not uses SSL. WARNING! Very unsecure\n"
								 "\n");
}

void free_onion(){
	fprintf(stderr,"\nClosing connections.\n");
	onion_free(o);
	exit(0);
}

int main(int argc, char **argv){
	int port=8080;
	const char *certificatefile="/etc/pki/tls/certs/pound.pem";
	const char *keyfile="/etc/pki/tls/certs/pound.key";
	int error;
	int i;
	int ssl=1;
	
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
			fprintf(stderr, "Using certificate %s\n",certificatefile);
		}
		else if(strcmp(argv[i],"-k")==0 || strcmp(argv[i],"--key")==0){
			if (i+1>argc){
				fprintf(stderr, "Need to set the certificate key filename.\n");
				show_help();
				exit(1);
			}
			keyfile=argv[++i];
			fprintf(stderr, "Using certificate key %s\n",keyfile);
		}
		else if(strcmp(argv[i],"--no-ssl")==0){
			ssl=0;
			fprintf(stderr, "Disabling SSL!\n");
		}
	}
	
	onion_handler *dir;
#ifdef __DEBUG__
	if (getenv("OTERM_DEBUG"))
		dir=onion_handler_directory(".");
	else{
		dir=onion_handler_opack("/",opack_index_html, opack_index_html_length);
		onion_handler_add(dir, onion_handler_opack("/jquery-1.4.3.min.js",opack_jquery_1_4_3_min_js,opack_jquery_1_4_3_min_js_length));
	}
#else
	dir=onion_handler_opack("/",opack_index_html, opack_index_html_length);
	onion_handler_add(dir, onion_handler_opack("/jquery-1.4.3.min.js",opack_jquery_1_4_3_min_js,opack_jquery_1_4_3_min_js_length));
#endif
	onion_handler_add(dir, onion_handler_path("^/term/",oterm_handler_data()));
	onion_handler_add(dir, onion_handler_static(NULL,"<h1>404 - File not found.</h1>", 404) );

	onion_handler *oterm=onion_handler_auth_pam("Onion Terminal", "login", dir);
	
	o=onion_new(O_THREADED);
	onion_set_root_handler(o, oterm);

	if (!(onion_flags(o)&O_SSL_AVAILABLE)){
		fprintf(stderr,"SSL support is not available. Oterm is in unsecure mode!\n");
	}
	else if (ssl){ // Not necesary the else, as onion_use_certificate would just return an error. But then it will exit.
		error=onion_set_certificate(o, O_SSL_CERTIFICATE_KEY, certificatefile, keyfile);
		if (error){
			fprintf(stderr, "Cant set certificate and key files (%s, %s)\n",certificatefile, keyfile);
			show_help();
			exit(1);
		}
	}
	
	onion_set_port(o, port);
	
	signal(SIGINT, free_onion);
	signal(SIGPIPE, SIG_IGN);
	fprintf(stderr, "Listening at %d\n",port);
	error=onion_listen(o);
	if (error){
		perror("Cant create the server");
	}
	
	onion_free(o);
	
	return 0;
}
