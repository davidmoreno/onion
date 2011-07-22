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

#include <onion/handlers/static.h>
#include <onion/handlers/opack.h>

#ifdef HAVE_PAM
#include <onion/handlers/auth_pam.h>
#endif

#ifdef __DEBUG__
#include <onion/handlers/exportlocal.h>
#endif 

#include "oterm_handler.h"
#include <onion/dict.h>


void opack_index_html(onion_response *res);
void opack_jquery_1_4_3_min_js(onion_response *res);
void opack_coralbits_png(onion_response *res);
extern unsigned int opack_index_html_length;
extern unsigned int opack_jquery_1_4_3_min_js_length;
extern unsigned int opack_coralbits_png_length;


onion *o=NULL;

/// Prints some help to stderr
void show_help(){
	fprintf(stderr," oterm - Linux terminal over HTTP\n\n"
								 "It uses HTTP(S)+HTML+CSS+JavaScript to show a remote terminal inside a browser.\n\n"
								 "Options:\n"
								 "   -p|--port <port_number>      -- Set the port number to use. Default 8080\n"
								 "   -i|--ip   <server_ip>        -- Set the ip to attach to. Default ::\n"
								 "   -c|--cert <certificate file> -- Set the SSL certificate file. Default: /etc/pki/tls/certs/pound.pem\n"
								 "   -k|--key  <key file>         -- Set the SSL key file. Default: /etc/pki/tls/certs/pound.key\n"
								 "   --no-pam                     -- Disable auth. Uses current user.\n"
								 "   --no-ssl                     -- Do not uses SSL. WARNING! Very unsecure\n"
								 "   -x|--exec <command>          -- On new terminals it executes this command instead of bash\n"
								 "\n");
}

void free_onion(){
	static int already_closing=0;
	if (!already_closing){
		fprintf(stderr,"\nClosing connections.\n");
		onion_free(o);
		already_closing=1;
	}
	exit(0);
}

int oterm_nopam(onion_handler *next, onion_request *req, onion_response *res){
	onion_dict *session=onion_request_get_session_dict(req);
	onion_dict_lock_write(session);
	onion_dict_add(session, "username", getenv("USER"), 0);
	onion_dict_add(session, "nopam", "true", 0);
	onion_dict_unlock(session);
	
	return onion_handler_handle(next, req, res);
}

int main(int argc, char **argv){
	char *port="8080";
	char *serverip="::";
	const char *command="/bin/bash";
	const char *certificatefile="/etc/pki/tls/certs/pound.pem";
	const char *keyfile="/etc/pki/tls/certs/pound.key";
	int error;
	int i;
	int ssl=1;
	int use_pam=1;
	
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
			port=argv[++i];
			fprintf(stderr, "Using port %s\n",port);
		}
		else if(strcmp(argv[i],"-i")==0 || strcmp(argv[i],"--ip")==0){
			if (i+1>argc){
				fprintf(stderr, "Need to set the ip address or hostname.\n");
				show_help();
				exit(1);
			}
			serverip=argv[++i];
			fprintf(stderr, "Using ip %s\n",serverip);
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
		else if(strcmp(argv[i],"-x")==0 || strcmp(argv[i],"--exec")==0){
			if (i+1>argc){
				fprintf(stderr, "Need the command to execute.\n");
				show_help();
				exit(1);
			}
			command=argv[++i];
			fprintf(stderr, "New terminal execute the command %s\n",command);
		}
		else if(strcmp(argv[i],"--no-ssl")==0){
			ssl=0;
			fprintf(stderr, "Disabling SSL!\n");
		}
		else if(strcmp(argv[i],"--no-pam")==0){
			use_pam=0;
			fprintf(stderr, "Disabling PAM!\n");
		}
	}
	
	onion_url *url=onion_url_new();
#ifdef __DEBUG__
	if (getenv("OTERM_DEBUG"))
		onion_url_add_handler(url, ".*", onion_handler_export_local_new("."));
	else{
		onion_url_add_handler(url, "^$", onion_handler_opack("",opack_index_html, opack_index_html_length));
		onion_url_add_handler(url, "^coralbits.png$", onion_handler_opack("",opack_coralbits_png, opack_coralbits_png_length));
		onion_url_add_handler(url, "^jquery-1.4.3.min.js$", onion_handler_opack("",opack_jquery_1_4_3_min_js, opack_jquery_1_4_3_min_js_length));
	}
#else
	onion_url_add_handler(url, "^$", onion_handler_opack("",opack_index_html, opack_index_html_length));
	onion_url_add_handler(url, "^coralbits.png$", onion_handler_opack("",opack_coralbits_png, opack_coralbits_png_length));
	onion_url_add_handler(url, "^jquery-1.4.3.min.js$", onion_handler_opack("",opack_jquery_1_4_3_min_js, opack_jquery_1_4_3_min_js_length));
#endif
	onion_url_add_handler(url, "^term/", oterm_handler(command));

	onion_handler *oterm;
#ifdef HAVE_PAM
	if (use_pam)
		oterm=onion_handler_auth_pam("Onion Terminal", "login", onion_url_to_handler(url));
	else
#endif
		oterm=onion_handler_new((void*)oterm_nopam, url, (void*)onion_handler_free);
	
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
	onion_set_hostname(o, serverip);
	
	signal(SIGINT, free_onion);
	signal(SIGPIPE, SIG_IGN);
	fprintf(stderr, "Listening at %s\n",port);
	error=onion_listen(o);
	if (error){
		perror("Cant create the server");
	}
	
	onion_free(o);
	
	return 0;
}
