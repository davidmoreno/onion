/*
	Onion HTTP server library
	Copyright (C) 2011 David Moreno Montero

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

#ifndef __ONION_POLLER_H__
#define __ONION_POLLER_H__

onion_poller *onion_poller_new(int aprox_n);
void onion_poller_free(onion_poller *);
int onion_poller_add(onion_poller *poller, int fd, void (*f)(void*), void *data);
int onion_poller_remove(onion_poller *poller, int fd);

void onion_poller_poll(onion_poller *);

void onion_poller_stop(onion_poller *);

#endif

