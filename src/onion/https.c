/*
	Onion HTTP server library
	Copyright (C) 2012 David Moreno Montero

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 3.0 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not see <http://www.gnu.org/licenses/>.
	*/

#include <gcrypt.h>
#include <gnutls/gnutls.h>
#include <malloc.h>
#include <stdarg.h>
#include <errno.h>

#include "https.h"
#include "http.h"
#include "types_internal.h"
#include "log.h"
#include "listen_point.h"
#include "request.h"

#ifdef HAVE_PTHREADS
#include <pthread.h>
GCRY_THREAD_OPTION_PTHREAD_IMPL;
#endif

struct onion_https_t{
	gnutls_certificate_credentials_t x509_cred;
	gnutls_dh_params_t dh_params;
	gnutls_priority_t priority_cache;
};

typedef struct onion_https_t onion_https;


int onion_http_read_ready(onion_request *req);
onion_request *onion_https_request_new(onion_listen_point *op);
static ssize_t onion_https_read(onion_request *req, char *data, size_t len);
static ssize_t onion_https_write(onion_request *req, const char *data, size_t len);
static void onion_https_close(onion_request *req);
static void onion_https_free(onion_listen_point *op);

onion_listen_point *onion_https_new(onion_ssl_certificate_type type, const char *filename, ...){
	onion_listen_point *op=onion_listen_point_new();
	op->request_new=onion_https_request_new;
	op->user_data=calloc(1,sizeof(onion_https));
	op->free=onion_https_free;
	op->read=onion_https_read;
	op->write=onion_https_write;
	op->close=onion_https_close;
	op->read_ready=onion_http_read_ready;
	
	onion_https *https=(onion_https*)op->user_data;
	
#ifdef HAVE_PTHREADS
	gcry_control (GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread);
#endif
	/*
	if (!(o->flags&O_USE_DEV_RANDOM)){
		gcry_control(GCRYCTL_ENABLE_QUICK_RANDOM, 0);
	}
	*/
	gnutls_global_init ();
	gnutls_certificate_allocate_credentials (&https->x509_cred);
	gnutls_dh_params_init (&https->dh_params);
	gnutls_dh_params_generate2 (https->dh_params, 1024);
	gnutls_certificate_set_dh_params (https->x509_cred, https->dh_params);
	gnutls_priority_init (&https->priority_cache, "PERFORMANCE:%SAFE_RENEGOTIATION:-VERS-TLS1.0", NULL);
	
	int r=0;
	switch(type&0x0FF){
		case O_SSL_CERTIFICATE_CRL:
			r=gnutls_certificate_set_x509_crl_file(https->x509_cred, filename, (type&O_SSL_DER) ? GNUTLS_X509_FMT_DER : GNUTLS_X509_FMT_PEM);
			break;
		case O_SSL_CERTIFICATE_KEY:
		{
			va_list va;
			va_start(va, filename);
			r=gnutls_certificate_set_x509_key_file(https->x509_cred, filename, va_arg(va, const char *), 
																									(type&O_SSL_DER) ? GNUTLS_X509_FMT_DER : GNUTLS_X509_FMT_PEM);
			va_end(va);
		}
			break;
		case O_SSL_CERTIFICATE_TRUST:
			r=gnutls_certificate_set_x509_trust_file(https->x509_cred, filename, (type&O_SSL_DER) ? GNUTLS_X509_FMT_DER : GNUTLS_X509_FMT_PEM);
			break;
		case O_SSL_CERTIFICATE_PKCS12:
		{
			va_list va;
			va_start(va, filename);
			r=gnutls_certificate_set_x509_simple_pkcs12_file(https->x509_cred, filename,
																														(type&O_SSL_DER) ? GNUTLS_X509_FMT_DER : GNUTLS_X509_FMT_PEM,
																														va_arg(va, const char *));
			va_end(va);
		}
			break;
		default:
			r=-1;
			ONION_ERROR("Set unknown type of certificate: %d",type);
	}
	
	if (r<0){
	  ONION_ERROR("Error setting the certificate (%s)", gnutls_strerror (r));
		onion_listen_point_free(op);
		return NULL;
	}
	
	
	ONION_DEBUG("HTTPS connection ready");
	
	return op;
}


static void onion_https_free(onion_listen_point *op){
	ONION_DEBUG("Close HTTPS %s:%s", op->hostname, op->port);
	onion_https *https=(onion_https*)op->user_data;
	
	gnutls_certificate_free_credentials (https->x509_cred);
	gnutls_dh_params_deinit(https->dh_params);
	gnutls_priority_deinit (https->priority_cache);
	//if (op->server->flags&O_SSL_NO_DEINIT)
		gnutls_global_deinit(); // This may cause problems if several characters use the gnutls on the same binary.
	free(https);
	shutdown(op->listenfd,SHUT_RDWR);
	close(op->listenfd);
}

onion_request* onion_https_request_new(onion_listen_point* op){
	onion_request *req=onion_listen_point_request_new_from_socket(op);
	onion_https *https=(onion_https*)op->user_data;
	
	ONION_DEBUG("Socket fd %d",req->connection.fd);
	
	gnutls_session_t session;

  gnutls_init (&session, GNUTLS_SERVER);
  gnutls_priority_set (session, https->priority_cache);
  gnutls_credentials_set (session, GNUTLS_CRD_CERTIFICATE, https->x509_cred);
  /* request client certificate if any.
   */
  gnutls_certificate_server_set_request (session, GNUTLS_CERT_REQUEST);
  /* Set maximum compatibility mode. This is only suggested on public webservers
   * that need to trade security for compatibility
   */
  gnutls_session_enable_compatibility_mode (session);

	gnutls_transport_set_ptr (session, (gnutls_transport_ptr_t)(long) req->connection.fd);
	int ret;
	do{
			ret = gnutls_handshake (session);
	}while (ret < 0 && gnutls_error_is_fatal (ret) == 0);
	if (ret<0){ // could not handshake. assume an error.
	  ONION_ERROR("Handshake has failed (%s)", gnutls_strerror (ret));
		gnutls_bye (session, GNUTLS_SHUT_WR);
		gnutls_deinit(session);
		onion_listen_point_request_close_socket(req);
		return NULL;
	}
	
	req->connection.user_data=(void*)session;
	
	return req;
}

static ssize_t onion_https_read(onion_request *req, char *data, size_t len){
	gnutls_session_t session=(gnutls_session_t)req->connection.user_data;
	ssize_t ret=gnutls_record_recv(session, data, len);
	ONION_DEBUG("Read! (%p), %d bytes", session, ret);
	if (ret<0){
	  ONION_ERROR("Reading data has failed (%s)", gnutls_strerror (ret));
	}
	return ret;
}

static ssize_t onion_https_write(onion_request *req, const char *data, size_t len){
	gnutls_session_t session=(gnutls_session_t)req->connection.user_data;
	ONION_DEBUG("Write! (%p)", session);
	return gnutls_record_send(session, data, len);
}

static void onion_https_close(onion_request *req){
	ONION_DEBUG("Close HTTPS connection");
	gnutls_session_t session=(gnutls_session_t)req->connection.user_data;
	if (session){
		ONION_DEBUG("Free session %p", session);
		gnutls_bye (session, GNUTLS_SHUT_WR);
		gnutls_deinit(session);
	
	}
	onion_listen_point_request_close_socket(req);
}
