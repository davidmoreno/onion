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
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <libgen.h>

#ifdef HAVE_GNUTLS
#include <gcrypt.h>		/* for gcry_control */
#include <gnutls/gnutls.h>
#endif
#ifdef HAVE_PTHREADS
#include <pthread.h>
#endif

#ifdef HAVE_GNUTLS
#ifdef HAVE_PTHREADS
GCRY_THREAD_OPTION_PTHREAD_IMPL;
#endif
#endif

#include "onion.h"
#include "onion_handler.h"
#include "onion_server.h"
#include "onion_types_internal.h"



#ifdef HAVE_GNUTLS
static gnutls_session_t onion_prepare_gnutls_session(onion *o, int clientfd);
static void onion_enable_tls(onion *o);
#endif

#ifdef HAVE_PTHREADS
/// Internal data as needed by onion_request_thread
struct onion_request_thread_data_t{
	onion *o;
	int clientfd;
};

typedef struct onion_request_thread_data_t onion_request_thread_data;

void *onion_request_thread(void*);
#endif

/// Internal processor of just one request.
static void onion_process_request(onion *o, int clientfd);

/// Creates the onion structure to fill with the server data, and later do the onion_listen()
onion *onion_new(int flags){
	onion *o=malloc(sizeof(onion));
	o->flags=flags&0x0FF;
	o->listenfd=0;
	o->server=malloc(sizeof(onion_server));
	o->server->root_handler=NULL;
	o->port=8080;
#ifdef HAVE_GNUTLS
	o->flags|=O_SSL_AVAILABLE;
#endif
#ifdef HAVE_PTHREADS
	o->flags|=O_THREADS_AVALIABLE;
	if (o->flags&O_THREADED){
		o->flags|=O_THREADS_ENABLED;
		o->active_threads_count=0;
		pthread_mutex_init(&o->mutex, NULL);
	}
#endif
	
	return o;
}

static int write_to_socket(int *fd, const char *data, unsigned int len){
	return write(*fd, data, len);
}

/**
 * @short Performs the listening with the given mode
 *
 * This is the main loop for the onion server.
 *
 * Returns if there is any error. It returns actualy errno from the network operations. See socket for more information.
 */
int onion_listen(onion *o){
#ifdef HAVE_PTHREADS
	if (o->flags&O_DETACH_LISTEN && !(o->flags&O_DETACHED)){ // On first call it sets the variable, and then call again, this time detached.
		o->flags|=O_DETACHED;
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); // It do not need to pthread_join. No leak here.
		pthread_t listen_thread;
		pthread_create(&listen_thread, &attr,(void*(*)(void*)) onion_listen, o);
		pthread_attr_destroy(&attr);
		return 0;
	}
#endif
	
	int sockfd;
	sockfd=socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in serv_addr, cli_addr;

	if (sockfd<0)
		return errno;
	memset((char *) &serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(o->port);
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		return errno;
	listen(sockfd,1);
  socklen_t clilen = sizeof(cli_addr);

	if (o->flags&O_ONE){
		int clientfd;
		if (o->flags&O_ONE_LOOP){
			while(1){
				clientfd=accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
				onion_process_request(o, clientfd);
				close(clientfd);
			}
		}
		else{
			clientfd=accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
			onion_process_request(o, clientfd);
			close(clientfd);
		}
	}
#ifdef HAVE_PTHREADS
	else if (o->flags&O_THREADS_ENABLED){
		int clientfd;
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); // It do not need to pthread_join. No leak here.
		while(1){
			clientfd=accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
			
			// Has to be malloc'd. If not it wil be overwritten on next petition. The threads frees it
			onion_request_thread_data *data=malloc(sizeof(onion_request_thread_data)); 
			data->o=o;
			data->clientfd=clientfd;
			pthread_t thread_handle; // Ignored just now. FIXME. Necesary at shutdown.
			pthread_mutex_lock (&o->mutex);
			o->active_threads_count++;
			pthread_mutex_unlock (&o->mutex);
			
			pthread_create(&thread_handle,&attr, onion_request_thread, data);
			}
		pthread_attr_destroy(&attr);
	}
#endif
	close(sockfd);
	return 0;
}

/// Removes the allocated data
void onion_free(onion *onion){
#ifdef HAVE_PTHREADS
	if (onion->flags&O_THREADS_ENABLED){
		int ntries=5;
		int c;
		for(;ntries--;){
			pthread_mutex_lock (&onion->mutex);
			c=onion->active_threads_count;
			pthread_mutex_unlock (&onion->mutex);
			if (c==0){
				break;
			}
			fprintf(stderr,"Still some petitions on process (%d). Wait a little bit (%d).\n",c,ntries);
			sleep(1);
		}
	}
#endif
	
#ifdef HAVE_GNUTLS
	if (onion->flags&O_SSL_ENABLED){
		gnutls_certificate_free_credentials (onion->x509_cred);
		gnutls_dh_params_deinit(onion->dh_params);
		gnutls_priority_deinit (onion->priority_cache);
		if (!(onion->flags&O_SSL_NO_DEINIT))
			gnutls_global_deinit(); // This may cause problems if several characters use the gnutls on the same binary.
	}
#endif
	onion_server_free(onion->server);
	free(onion);
}

/// Sets the root handler
void onion_set_root_handler(onion *onion, onion_handler *handler){
	onion->server->root_handler=handler;
}

void onion_set_port(onion *server, int port){
	server->port=port;
}

/**
 * @short Internal processor of just one request.
 *
 * It can be used on one processing, on threaded, on one_loop...
 */
static void onion_process_request(onion *o, int clientfd){
	// sorry all the ifdefs, but here is the worst case where i would need it.. and both are almost the same.
#ifdef HAVE_GNUTLS
	gnutls_session_t session;
	if (o->flags&O_SSL_ENABLED){
		session=onion_prepare_gnutls_session(o, clientfd);
		if (session==NULL)
			return;
	}
#endif
	int r,w;
	char buffer[1024];
	onion_request *req;
#ifdef HAVE_GNUTLS
	if (o->flags&O_SSL_ENABLED){
		onion_server_set_write(o->server, (onion_write)gnutls_record_send); // Im lucky, has the same signature.
		req=onion_request_new(o->server, session);
	}
	else{
		onion_server_set_write(o->server, (onion_write)write_to_socket);
		req=onion_request_new(o->server, &clientfd);
	}
	while ( ( r = (o->flags&O_SSL_ENABLED)
							? gnutls_record_recv (session, buffer, sizeof(buffer))
							: read(clientfd, buffer, sizeof(buffer))
					) >0 ){
#else
	req=onion_request_new(o->server, &clientfd);
	onion_server_set_write(o->server, (onion_write)write_to_socket);
	while ( ( r=read(clientfd, buffer, sizeof(buffer)) ) >0){
#endif
		//fprintf(stderr, "%s:%d Read %d bytes\n",__FILE__,__LINE__,r);
		w=onion_request_write(req, buffer, r);
		if (w<0){ // request processed.
			break;
		}
	}
	onion_request_free(req);
#ifdef HAVE_GNUTLS
	if (o->flags&O_SSL_ENABLED){
		gnutls_bye (session, GNUTLS_SHUT_WR);
		gnutls_deinit (session);
	}
#endif
}

/// Returns the current flags
int onion_flags(onion *onion){
	return onion->flags;
}


#ifdef HAVE_GNUTLS
static gnutls_session_t onion_prepare_gnutls_session(onion *o, int clientfd){
	gnutls_session_t session;

  gnutls_init (&session, GNUTLS_SERVER);
  gnutls_priority_set (session, o->priority_cache);
  gnutls_credentials_set (session, GNUTLS_CRD_CERTIFICATE, o->x509_cred);
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
	  fprintf (stderr, "%s:%d Handshake has failed (%s)\n", basename(__FILE__), __LINE__, gnutls_strerror (ret));
		gnutls_deinit (session);
		return NULL;
	}
	return session;
}

static void onion_enable_tls(onion *o){
#ifdef HAVE_GNUTLS
#ifdef HAVE_PTHREADS
	gcry_control (GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread);
#endif
#endif
	if (!(o->flags&O_USE_DEV_RANDOM)){
		gcry_control(GCRYCTL_ENABLE_QUICK_RANDOM, 0);
	}
	gnutls_global_init ();
  gnutls_certificate_allocate_credentials (&o->x509_cred);

	gnutls_dh_params_init (&o->dh_params);
  gnutls_dh_params_generate2 (o->dh_params, 1024);
  gnutls_certificate_set_dh_params (o->x509_cred, o->dh_params);
  gnutls_priority_init (&o->priority_cache, "NORMAL", NULL);
	
	o->flags|=O_SSL_ENABLED;
}
#endif


/**
 * @short Set a certificate for use in the connection
 *
 * Returns the error code. If 0, no error.
 */
int onion_use_certificate(onion *onion, onion_ssl_certificate_type type, const char *filename, ...){
#ifdef HAVE_GNUTLS
	if (!(onion->flags&O_SSL_ENABLED))
		onion_enable_tls(onion);
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
#else
	return -1; /// Support not available
#endif
}


#ifdef HAVE_PTHREADS
/// Interfaces between the pthread_create and the process request. Actually just calls it with the proper parameters.
void *onion_request_thread(void *d){
	onion_request_thread_data *td=(onion_request_thread_data*)d;
	
	onion_process_request(td->o,td->clientfd);
	
	close(td->clientfd);
	pthread_mutex_lock (&td->o->mutex);
	td->o->active_threads_count--;
	pthread_mutex_unlock (&td->o->mutex);
	free(td);
	return NULL;
}

#endif
