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

#include <malloc.h>
#include <stdio.h>
#include <libgen.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>


#include <gcrypt.h>		/* for gcry_control */
#include <gnutls/gnutls.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#include <onion_types_internal.h>

#include <onion.h>
#include <onion_handler.h>
#include <onion_server.h>

#include <onion_ssl.h>
#include <onion_ssl_internal.h>

static void onion_ssl_process_request(onion_ssl *server, int clientfd);

// This file is quite similar to onion.c, and I got long time thinking if Im copying funcitonality and
// if I can not stop duplicating functionality. The easy answer is yes, I can, but it does not worth.
// At least by the moment.

/// Creates the onion structure to fill with the server data, and later do the onion_listen()
onion_ssl *onion_ssl_new(int flags){
	onion_ssl *o=malloc(sizeof(onion_ssl));
	o->o=onion_new(flags);
	
	if (!(flags&O_USE_DEV_RANDOM)){
		gcry_control (GCRYCTL_ENABLE_QUICK_RANDOM, 0);
	}
	gnutls_global_init ();
  gnutls_certificate_allocate_credentials (&o->x509_cred);

	gnutls_dh_params_init (&o->dh_params);
  gnutls_dh_params_generate2 (o->dh_params, 1024);
  gnutls_certificate_set_dh_params (o->x509_cred, o->dh_params);
  gnutls_priority_init (&o->priority_cache, "NORMAL", NULL);

	return o;
}


/// Set a certificate for use in the connection
int onion_ssl_use_certificate(onion_ssl *onion, onion_ssl_certificate_type type, const char *filename, ...){
	int r=-1;
	switch(type&0x0FF){
		case O_SSL_CERTIFICATE_CRL:
			r=gnutls_certificate_set_x509_crl_file(onion->x509_cred, filename, (type&O_SSL_DER) ? GNUTLS_X509_FMT_DER : GNUTLS_X509_FMT_PEM);
			break;
		case O_SSL_CERTIFICATE_KEY:
		{
			va_list va;
			va_start(va, filename);
			r=gnutls_certificate_set_x509_key_file(onion->x509_cred, filename, va_arg(va, const char *), 
																									(type&O_SSL_DER) ? GNUTLS_X509_FMT_DER : GNUTLS_X509_FMT_PEM);
			va_end(va);
		}
			break;
		case O_SSL_CERTIFICATE_TRUST:
			r=gnutls_certificate_set_x509_trust_file(onion->x509_cred, filename, (type&O_SSL_DER) ? GNUTLS_X509_FMT_DER : GNUTLS_X509_FMT_PEM);
			break;
		case O_SSL_CERTIFICATE_PKCS12:
		{
			va_list va;
			va_start(va, filename);
			r=gnutls_certificate_set_x509_simple_pkcs12_file(onion->x509_cred, filename,
																														(type&O_SSL_DER) ? GNUTLS_X509_FMT_DER : GNUTLS_X509_FMT_PEM,
																														va_arg(va, const char *));
			va_end(va);
		}
			break;
		default:
			fprintf(stderr,"%s:%d Set unknown type of certificate: %d\n", basename(__FILE__), __LINE__, type);
	}
	if (r<0){
	  fprintf (stderr, "%s:%d Error setting the certificate (%s)\n\n", basename(__FILE__), __LINE__,
		   gnutls_strerror (r));
	}
	return r;
}

/// Performs the listening with the given mode
int onion_ssl_listen(onion_ssl *server){
	int sockfd;
	sockfd=socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in serv_addr, cli_addr;

	if (sockfd<0)
		return errno;
	memset((char *) &serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(server->o->port);
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		return errno;
	listen(sockfd,1);
  socklen_t clilen = sizeof(cli_addr);

	if (server->o->flags&O_ONE){
		int clientfd;
		if (server->o->flags&O_ONE_LOOP){
			while(1){
				clientfd=accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
				onion_ssl_process_request(server, clientfd);
				close(clientfd);
			}
		}
		else{
			clientfd=accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
			onion_ssl_process_request(server, clientfd);
			close(clientfd);
		}
	}
	close(sockfd);
	return 0;
}

/// Removes the allocated data
void onion_ssl_free(onion_ssl *onion){
	gnutls_certificate_free_credentials (onion->x509_cred);
	gnutls_dh_params_deinit(onion->dh_params);
	if (!(onion->o->flags&O_SSL_NO_DEINIT))
		gnutls_global_deinit(); // This may cause problems if several characters use the gnutls on the same binary.
	onion_free(onion->o);
	free(onion);
}

/// Sets the root handler
void onion_ssl_set_root_handler(onion_ssl *server, onion_handler *handler){
	onion_set_root_handler(server->o, handler);
}

/// Sets the port to listen
void onion_ssl_set_port(onion_ssl *server, int port){
	onion_set_port(server->o, port);
}

#if 0
/// Simple wrapper
static int write_to_socket_ssl(gnutls_session_t session, const char *data, unsigned int len){
	return gnutls_record_send (session, data, len);
}
#endif

void onion_ssl_process_request(onion_ssl *server, int clientfd){
	gnutls_session_t session;

  gnutls_init (&session, GNUTLS_SERVER);
  gnutls_priority_set (session, server->priority_cache);
  gnutls_credentials_set (session, GNUTLS_CRD_CERTIFICATE, server->x509_cred);
  /* request client certificate if any.
   */
  gnutls_certificate_server_set_request (session, GNUTLS_CERT_REQUEST);
  /* Set maximum compatibility mode. This is only suggested on public webservers
   * that need to trade security for compatibility
   */
  gnutls_session_enable_compatibility_mode (session);

	gnutls_transport_set_ptr (session, (gnutls_transport_ptr_t)(long) clientfd);
	int ret = gnutls_handshake (session);
	if (ret<0){ // could not handshake. assume an error.
	  fprintf (stderr, "%s:%d Handshake has failed (%s)\n\n", basename(__FILE__), __LINE__,
		   gnutls_strerror (ret));
		return;
	}
	
	int r,w;
	char buffer[1024];
	onion_request *req=onion_request_new(server->o->server, session);
	onion_server_set_write(server->o->server, (onion_write)gnutls_record_send); // Im lucky, has the same signature.
	while ( ( r=gnutls_record_recv (session, buffer, sizeof(buffer)) ) >0){
		//fprintf(stderr, "%s:%d Read %d bytes\n",__FILE__,__LINE__,r);
		w=onion_request_write(req, buffer, r);
		if (w<0){ // request processed.
			onion_request_free(req);
			gnutls_bye (session, GNUTLS_SHUT_WR);
      gnutls_deinit (session);
			return;
		}
	}
	/// error on transmission or other end closed...
	gnutls_deinit (session);
}
