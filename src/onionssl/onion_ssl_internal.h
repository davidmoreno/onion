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
#ifndef __ONION_SSL_INTERNAL__
#define __ONION_SSL_INTERNAL__

#ifdef __cplusplus
extern "C"{
#endif

#include <gnutls/gnutls.h>

#include <onion_types.h>

/// Basic data for the SSL connections
struct onion_ssl_t{
	onion *o; /// first the original onion structure, as all in there fits here.
	gnutls_certificate_credentials_t x509_cred;
	gnutls_dh_params_t dh_params;
	gnutls_priority_t priority_cache;
};

#ifdef __cplusplus
extern "C"{
#endif

#endif
