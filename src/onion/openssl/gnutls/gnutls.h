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

	You should have received a copy of both libraries, if not see 
	<http://www.gnu.org/licenses/> and 
	<http://www.apache.org/licenses/LICENSE-2.0>.
	*/

#ifndef ONION_OPENSSL_GNUTLS_H
#define ONION_OPENSSL_GNUTLS_H

#include <openssl/ssl.h>
#include <openssl/crypto.h>
#include <openssl/engine.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

#ifdef __cplusplus
extern "C"{
#endif

#define GNUTLS_VERSION_NUMBER	0x000000
#define GNUTLS_X509_FMT_DER	SSL_FILETYPE_ASN1
#define GNUTLS_X509_FMT_PEM	SSL_FILETYPE_PEM

#define gnutls_strerror(...)	ERR_error_string(ERR_get_error(), NULL)
#define gnutls_record_recv	SSL_read
#define gnutls_record_send	SSL_write
#define gnutls_bye(S, ...)	SSL_shutdown(S)
#define gnutls_deinit		SSL_free

#define gnutls_certificate_set_x509_crl_file(...)		-1
#define gnutls_certificate_set_x509_trust_file(...)		-1
#define gnutls_certificate_set_x509_simple_pkcs12_file(...)	-1

#define gnutls_global_init() do {		\
	SSL_library_init();			\
	SSL_load_error_strings(); } while(0)

#define gnutls_global_deinit() do {		\
	FIPS_mode_set(0);			\
	ERR_remove_thread_state(NULL);		\
	SSL_COMP_free_compression_methods();	\
	ENGINE_cleanup();			\
	CONF_modules_free();			\
	CONF_modules_unload(1);			\
	COMP_zlib_cleanup();			\
	ERR_free_strings();			\
	EVP_cleanup();				\
	CRYPTO_cleanup_all_ex_data(); } while(0)

#define gnutls_certificate_set_x509_key_file(CTX, CRT, KEY, TYPE) ({	\
	int R=SSL_CTX_use_certificate_file(CTX, CRT, TYPE);		\
	if (R>0) R=SSL_CTX_use_PrivateKey_file(CTX, KEY, TYPE);		\
	(R - 1); })

typedef SSL *gnutls_session_t;

#ifdef __cplusplus
}
#endif

#endif
