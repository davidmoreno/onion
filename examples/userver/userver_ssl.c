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
#include <onion_handler_directory.h>
#include <onion_handler_static.h>

onion_ssl *o=NULL;

void free_onion(){
	fprintf(stderr,"\nClosing connections.\n");
	onion_ssl_free(o);
	exit(0);
}

int main(int argc, char **argv){
	onion_handler *dir=onion_handler_directory(argc==2 ? argv[1] : ".");
	onion_handler_add(dir, onion_handler_static(NULL,"<h1>404 - File not found.</h1>", 404) );
	
	o=onion_ssl_new(O_ONE);
	onion_ssl_set_root_handler(o, dir);
	int port=8080;
	if (getenv("ONION_PORT"))
		port=atoi(getenv("ONION_PORT"));
	
	onion_ssl_set_port(o, port);
	
	
	int ok=onion_ssl_use_certificate(o, O_SSL_CERTIFICATE_KEY, "/etc/pki/tls/certs/pound.pem", "/etc/pki/tls/certs/pound.key");
	if (ok<0){
		fprintf(stderr,"Error setting the certificate: %d\n",ok);
		exit(1);
	}
	
	signal(SIGINT, free_onion);
	int error=onion_ssl_listen(o);
	if (error){
		perror("Cant create the server");
	}
	
	onion_ssl_free(o);
	
	return 0;
}
