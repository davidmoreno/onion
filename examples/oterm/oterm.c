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
#include <time.h>
#include <errno.h>

#include <onion/log.h>
#include <onion/onion.h>
#include <onion/handler.h>

#include <onion/handlers/static.h>
#include <onion/handlers/opack.h>

#ifdef HAVE_PAM
#include <onion/handlers/auth_pam.h>
#endif

#include <onion/handlers/exportlocal.h>

#include "oterm_handler.h"
#include <onion/dict.h>
#include <onion/shortcuts.h>

#include <assets.h>

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

void free_onion(int unused){
	static int already_closing=0;
	if (!already_closing){
		ONION_INFO("Closing connections.");
		onion_free(o);
		already_closing=1;
	}
	exit(0);
}

int oterm_nopam(onion_handler *next, onion_request *req, onion_response *res){
	onion_dict *session=onion_request_get_session_dict(req);
	onion_dict_lock_write(session);
	const char *username=getenv("USER");
	if (username)
		onion_dict_add(session, "username", username, 0);
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
#ifdef HAVE_PAM
	int use_pam=1;
#endif
	
	for (i=1;i<argc;i++){
		if (strcmp(argv[i],"--help")==0){
			show_help();
			exit(0);
		}
		else if(strcmp(argv[i],"-p")==0 || strcmp(argv[i],"--port")==0){
			if (i+1>argc){
				ONION_ERROR("Need to set the port number.");
				show_help();
				exit(1);
			}
			port=argv[++i];
			fprintf(stderr, "Using port %s\n",port);
		}
		else if(strcmp(argv[i],"-i")==0 || strcmp(argv[i],"--ip")==0){
			if (i+1>argc){
				ONION_ERROR("Need to set the ip address or hostname.");
				show_help();
				exit(1);
			}
			serverip=argv[++i];
			fprintf(stderr, "Using ip %s\n",serverip);
		}
		else if(strcmp(argv[i],"-c")==0 || strcmp(argv[i],"--cert")==0){
			if (i+1>argc){
				ONION_ERROR("Need to set the certificate filename");
				show_help();
				exit(1);
			}
			certificatefile=argv[++i];
			ONION_INFO("Using certificate %s",certificatefile);
		}
		else if(strcmp(argv[i],"-k")==0 || strcmp(argv[i],"--key")==0){
			if (i+1>argc){
				ONION_ERROR("Need to set the certificate key filename.");
				show_help();
				exit(1);
			}
			keyfile=argv[++i];
			ONION_INFO("Using certificate key %s",keyfile);
		}
		else if(strcmp(argv[i],"-x")==0 || strcmp(argv[i],"--exec")==0){
			if (i+1>argc){
				ONION_ERROR("Need the command to execute.");
				show_help();
				exit(1);
			}
			command=argv[++i];
			ONION_INFO("New terminal execute the command %s",command);
		}
		else if(strcmp(argv[i],"--no-ssl")==0){
			ssl=0;
			ONION_INFO("Disabling SSL!");
		}
#ifdef HAVE_PAM
		else if(strcmp(argv[i],"--no-pam")==0){
			use_pam=0;
			ONION_INFO("Disabling PAM!");
		}
#endif
	}
  o=onion_new(O_POOL|O_SYSTEMD);
  
	
	// I prepare the url handler, with static, uuid and term. Also added the empty rule that redirects to static/index.html
	onion_url *url=onion_url_new();
  onion_handler *term_handler=oterm_handler(o,command);
#ifdef HAVE_PAM
  if (use_pam){
    onion_url_add_handler(url, "^term/", onion_handler_auth_pam("Onion Terminal", "login", term_handler));
  }
  else
#endif
  {
    onion_url_add_with_data(url, "^term/", oterm_nopam, term_handler, NULL);
  }
  onion_url_add_with_data(url, "^uuid/", oterm_uuid, onion_handler_get_private_data(term_handler), NULL);
  
#ifdef __DEBUG__
	if (getenv("OTERM_DEBUG"))
		onion_url_add_handler(url, "^static/", onion_handler_export_local_new("static"));
	else
#endif
  {
#ifndef JQUERY_JS
    onion_url_add(url, "^static/jquery.js", opack_jquery_js);
#else
    onion_url_add_handler(url, "^static/jquery.js", onion_handler_export_local_new(JQUERY_JS));
#endif
    onion_url_add(url, "^static/", opack_static);
	}
  onion_url_add_with_data(url, "", onion_shortcut_internal_redirect, "static/index.html", NULL);

  srand(time(NULL));
	onion_set_root_handler(o, onion_url_to_handler(url));

	if (!(onion_flags(o)&O_SSL_AVAILABLE)){
		ONION_WARNING("SSL support is not available. Oterm is in unsecure mode!");
	}
	else if (ssl){ // Not necesary the else, as onion_use_certificate would just return an error. But then it will exit.
		error=onion_set_certificate(o, O_SSL_CERTIFICATE_KEY, certificatefile, keyfile);
		if (error){
			ONION_ERROR("Cant set certificate and key files (%s, %s)",certificatefile, keyfile);
			show_help();
			exit(1);
		}
	}
	
	onion_set_port(o, port);
	onion_set_hostname(o, serverip);
  onion_set_timeout(o,5000);
	
	signal(SIGINT, free_onion);
	signal(SIGPIPE, SIG_IGN);
	fprintf(stderr, "Listening at %s\n",port);
	error=onion_listen(o);
	if (error){
		ONION_ERROR("Cant create the server: %s", strerror(errno));
	}
	
	onion_free(o);
	
	return 0;
}
