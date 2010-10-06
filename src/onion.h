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

#ifndef __ONION__
#define __ONION__

#ifdef __cplusplus
extern "C"{
#endif

enum onion_mode_e{
	ONE=1,
	WAIT=2,
	SELECT=3,
	THREADED=4
};

typedef enum onion_mode_e onion_mode;

/**
 * @short Basic structure that contains the webserver info.
 */
struct onion_t{
	int listenfd;
};

typedef struct onion_t onion;

/// Creates the onion structure to fill with the server data, and later do the onion_listen()
onion *onion_new();

/// Performs the listening with the given mode
int onion_listen(onion *server, onion_mode mode);

#ifdef __cplusplus
}
#endif


#endif
