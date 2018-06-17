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

#ifndef ONION_OPENSSL_GCRYPT_H
#define ONION_OPENSSL_GCRYPT_H

#include <openssl/evp.h>

#ifdef __cplusplus
extern "C"{
#endif

#define GCRYPT_VERSION_NUMBER	000000

#define GCRY_THREAD_OPTION_PTHREAD_IMPL

#define GCRY_MD_SHA1		EVP_sha1()

#define gcry_control(...)

#define gcry_md_hash_buffer(MD, R, D, L)	\
	EVP_Digest(D, L, (unsigned char *)R, NULL, MD, NULL)

#define gcry_create_nonce	RAND_pseudo_bytes

#ifdef __cplusplus
}
#endif

#endif
