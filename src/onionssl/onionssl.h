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

#ifndef __ONION_SSL__
#define __ONION_SSL__

#ifdef __cplusplus
extern "C"{
#endif

/// Basic data for the SSL connections
struct onion_ssl_t{
};

typedef struct onion_ssl_t onion_ssl;

/// Creates the onion structure to fill with the server data, and later do the onion_listen()
onion_ssl *onion_ssl_new(int flags);

#ifdef __cplusplus
}
#endif

#endif
