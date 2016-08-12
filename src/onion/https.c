/*
	Onion HTTP server library
	Copyright (C) 2010-2016 David Moreno Montero and others

	This library is free software; you can redistribute it and/or
	modify it under the terms of, at your choice:

	a. the Apache License Version 2.0.

	b. the GNU General Public License as published by the
		Free Software Foundation; either version 2.0 of the License,
		or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of both licenses, if not see
	<http://www.gnu.org/licenses/> and
	<http://www.apache.org/licenses/LICENSE-2.0>.
	*/

#include <gcrypt.h>
#include <gnutls/gnutls.h>
/* Notice that malloc.h is deprecated, use stdlib.h instead */
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>

#include "low.h"
#include "https.h"
#include "http.h"
#include "types_internal.h"
#include "log.h"
#include "listen_point.h"
#include "request.h"

#ifdef HAVE_PTHREADS
#include <pthread.h>

#if GCRYPT_VERSION_NUMBER < 010600
GCRY_THREAD_OPTION_PTHREAD_IMPL;
#endif

#endif

/// @defgroup https HTTPS. Specific bits for https listen points. Use to set certificates.

/**
 * @short Stores some data about the connection
 * @struct onion_https_t
 * @memberof onion_https_t
 * @ingroup https
 *
 * It has the main data for the connection; the setup certificate and such.
 */
struct onion_https_t{
	gnutls_certificate_credentials_t x509_cred;
	gnutls_dh_params_t dh_params;
	gnutls_priority_t priority_cache;
};

typedef struct onion_https_t onion_https;


int onion_http_read_ready(onion_request *req);
static int onion_https_request_init(onion_request *req);
static ssize_t onion_https_read(onion_request *req, char *data, size_t len);
ssize_t onion_https_write(onion_request *req, const char *data, size_t len);
static void onion_https_close(onion_request *req);
static void onion_https_listen_stop(onion_listen_point *op);
static void onion_https_free_user_data(onion_listen_point *op);

/**
 * @short Creates a new listen point with HTTPS powers.
 * @memberof onion_https_t
 * @ingroup https
 *
 * Creates the HTTPS listen point.
 *
 * Might be called with (O_SSL_NONE,NULL), and set up the certificate later with onion_https_set_certificate.
 *
 * @param type Type of certificate to setup
 * @param filename File from where to get the data
 * @param ... More types and filenames until O_SSL_NONE.
 * @returns An onion_listen_point with the desired data, ready to start listening.
 */
onion_listen_point *onion_https_new(){
	onion_listen_point *op=onion_listen_point_new();
	op->request_init=onion_https_request_init;
	op->free_user_data=onion_https_free_user_data;
	op->listen_stop=onion_https_listen_stop;
	op->read=onion_https_read;
	op->write=onion_https_write;
	op->close=onion_https_close;
	op->read_ready=onion_http_read_ready;
	op->secure = true;

	op->user_data=onion_low_calloc(1,sizeof(onion_https));
	onion_https *https=(onion_https*)op->user_data;

#ifdef HAVE_PTHREADS
#if GCRYPT_VERSION_NUMBER < 010600
	gcry_control (GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread);
#endif
#endif
	//if (!(o->flags&O_USE_DEV_RANDOM)){
		gcry_control(GCRYCTL_ENABLE_QUICK_RANDOM, 0);
	//}

	gnutls_global_init ();
	gnutls_certificate_allocate_credentials (&https->x509_cred);

	// set cert here??
	//onion_https_set_certificate(op,O_SSL_CERTIFICATE_KEY, "mycert.pem","mycert.pem");
	int e;
#if GNUTLS_VERSION_NUMBER >= 0x020C00
	int bits = gnutls_sec_param_to_pk_bits (GNUTLS_PK_DH, GNUTLS_SEC_PARAM_LOW);
#else
	int bits = 1024;
#endif
	e=gnutls_dh_params_init (&https->dh_params);
	if (e<0){
		ONION_ERROR("Error initializing HTTPS: %s", gnutls_strerror(e));
		gnutls_certificate_free_credentials (https->x509_cred);
		op->free_user_data=NULL;
		onion_listen_point_free(op);
		onion_low_free(https);
		return NULL;
	}
	e=gnutls_dh_params_generate2 (https->dh_params, bits);
	if (e<0){
		ONION_ERROR("Error initializing HTTPS: %s", gnutls_strerror(e));
		gnutls_certificate_free_credentials (https->x509_cred);
		op->free_user_data=NULL;
		onion_listen_point_free(op);
		onion_low_free(https);
		return NULL;
	}
	e=gnutls_priority_init (&https->priority_cache, "PERFORMANCE:%SAFE_RENEGOTIATION:-VERS-TLS1.0", NULL);
	if (e<0){
		ONION_ERROR("Error initializing HTTPS: %s", gnutls_strerror(e));
		gnutls_certificate_free_credentials (https->x509_cred);
		gnutls_dh_params_deinit(https->dh_params);
		op->free_user_data=NULL;
		onion_listen_point_free(op);
		onion_low_free(https);
		return NULL;
	}
	gnutls_certificate_set_dh_params (https->x509_cred, https->dh_params);

	ONION_DEBUG("HTTPS connection ready");

	return op;
}

/**
 * @short Stop the listening.
 * @memberof onion_https_t
 * @ingroup https
 *
 * Just closes the listen port.
 *
 * @param op The listen port.
 */
static void onion_https_listen_stop(onion_listen_point *op){
	ONION_DEBUG("Close HTTPS %s:%s", op->hostname, op->port);
	shutdown(op->listenfd,SHUT_RDWR);
	close(op->listenfd);
	op->listenfd=-1;
}

/**
 * @short Frees the user data
 * @memberof onion_https_t
 * @ingroup https
 *
 * @param op
 */
static void onion_https_free_user_data(onion_listen_point *op){
	ONION_DEBUG("Free HTTPS %s:%s", op->hostname, op->port);
	onion_https *https=(onion_https*)op->user_data;

	gnutls_certificate_free_credentials (https->x509_cred);
	gnutls_dh_params_deinit(https->dh_params);
	gnutls_priority_deinit (https->priority_cache);
	//if (op->server->flags&O_SSL_NO_DEINIT)
	gnutls_global_deinit(); // This may cause problems if several characters use the gnutls on the same binary.
	onion_low_free(https);
}

/**
 * @short Initializes a connection on a request
 * @memberof onion_https_t
 * @ingroup https
 *
 * Do the accept of the request, and the SSL handshake.
 *
 * @param req The request
 * @returns <0 in case of error.
 */
static int onion_https_request_init(onion_request *req){
	onion_listen_point_request_init_from_socket(req);
	onion_https *https=(onion_https*)req->connection.listen_point->user_data;

	ONION_DEBUG("Accept new request, fd %d",req->connection.fd);

	gnutls_session_t session;

  gnutls_init (&session, GNUTLS_SERVER);
  gnutls_priority_set (session, https->priority_cache);
  gnutls_credentials_set (session, GNUTLS_CRD_CERTIFICATE, https->x509_cred);
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
		return -1;
	}

	req->connection.user_data=(void*)session;
	return 0;
}

/**
 * @short Method to read some HTTPS data.
 * @memberof onion_https_t
 * @ingroup https
 *
 * @param req to get data from
 * @param data where to store unencrypted data
 * @param Lenght of desired data
 * @returns Actual read data. 0 means EOF.
 */
static ssize_t onion_https_read(onion_request *req, char *data, size_t len){
	gnutls_session_t session=(gnutls_session_t)req->connection.user_data;
	ssize_t ret=gnutls_record_recv(session, data, len);
	ONION_DEBUG("Read! (%p), %d bytes", session, ret);
	if (ret<0){
	  ONION_ERROR("Reading data has failed (%s)", gnutls_strerror (ret));
	}
	return ret;
}

/**
 * @short Writes some data to the HTTPS client.
 * @memberof onion_https_t
 * @ingroup https
 *
 * @param req to where write the data
 * @param data to write
 * @param len Ammount of data desired to write
 * @returns Actual ammount of data written.
 */
ssize_t onion_https_write(onion_request *req, const char *data, size_t len){
	gnutls_session_t session=(gnutls_session_t)req->connection.user_data;
	ONION_DEBUG("Write! (%p)", session);
	return gnutls_record_send(session, data, len);
}

/**
 * @short Closes the https connection
 * @memberof onion_https_t
 * @ingroup https
 *
 * It frees local data and closes the socket.
 *
 * @param req to close.
 */
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

/**
 * @short Set new certificate elements
 * @memberof onion_https_t
 * @ingroup https
 *
 * @param ol Listen point
 * @param type Type of certificate to add
 * @param filename File where this data is.
 * @returns If the operation was sucesful
 */
int onion_https_set_certificate(onion_listen_point *ol, onion_ssl_certificate_type type, const char *filename, ...){
	va_list va;
	va_start(va, filename);
	int r=onion_https_set_certificate_argv(ol, type, filename, va);
	va_end(va);

	return r;
}

/**
 * @short Same as onion_https_set_certificate, but with a va_list
 * @memberof onion_https_t
 * @ingroup https
 *
 * This allows to manage va_lists more easily.
 *
 * @see onion_https_set_certificate
 */
int onion_https_set_certificate_argv(onion_listen_point *ol, onion_ssl_certificate_type type, const char *filename, va_list va){
	onion_https *https=(onion_https*)ol->user_data;

	if (ol->write!=onion_https_write){
		ONION_ERROR("Trying to et a certificate on a non HTTPS listen point");
		errno=EINVAL;
		return -1;
	}
	int r=0;
	switch(type&0x0FF){
		case O_SSL_CERTIFICATE_CRL:
			ONION_DEBUG("Setting SSL Certificate CRL");
			r=gnutls_certificate_set_x509_crl_file(https->x509_cred, filename, (type&O_SSL_DER) ? GNUTLS_X509_FMT_DER : GNUTLS_X509_FMT_PEM);
			break;
		case O_SSL_CERTIFICATE_KEY:
		{
			//va_arg(va, const char *); // Ignore first.
			const char *keyfile=va_arg(va, const char *);
			ONION_DEBUG("Setting certificate to %p: cert %s, key %s", ol, filename, keyfile);
			r=gnutls_certificate_set_x509_key_file(https->x509_cred, filename, keyfile,
																									(type&O_SSL_DER) ? GNUTLS_X509_FMT_DER : GNUTLS_X509_FMT_PEM);
		}
			break;
		case O_SSL_CERTIFICATE_TRUST:
			ONION_DEBUG("Setting SSL Certificate Trust");
			r=gnutls_certificate_set_x509_trust_file(https->x509_cred, filename, (type&O_SSL_DER) ? GNUTLS_X509_FMT_DER : GNUTLS_X509_FMT_PEM);
			break;
		case O_SSL_CERTIFICATE_PKCS12:
		{
			ONION_DEBUG("Setting SSL Certificate PKCS12");
			r=gnutls_certificate_set_x509_simple_pkcs12_file(https->x509_cred, filename,
																														(type&O_SSL_DER) ? GNUTLS_X509_FMT_DER : GNUTLS_X509_FMT_PEM,
																														va_arg(va, const char *));
		}
			break;
		default:
			r=-1;
			ONION_ERROR("Set unknown type of certificate: %d",type);
	}

	return r;
}
