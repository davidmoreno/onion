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

#ifndef ONION_POLLER_H
#define ONION_POLLER_H

#include "types.h"

#ifdef __cplusplus
extern "C"{
#endif

/// @ingroup poller
enum onion_poller_slot_type_e{
	O_POLL_READ=1,
	O_POLL_WRITE=2,
	O_POLL_OTHER=4,
	O_POLL_ALL=7
};

typedef enum onion_poller_slot_type_e onion_poller_slot_type_e;

/// Create a new slot for the poller
onion_poller_slot *onion_poller_slot_new(int fd, int (*f)(void*), void *data);
/// Cleans a poller slot. Do not call if already on the poller (onion_poller_add). Use onion_poller_remove instead.
void onion_poller_slot_free(onion_poller_slot *el);
/// Sets the shutdown function for this poller slot
void onion_poller_slot_set_shutdown(onion_poller_slot *el, void (*shutdown)(void*), void *data);
/// Sets the timeout for this slot. Current implementation takes ms, but then it rounds to seconds.
void onion_poller_slot_set_timeout(onion_poller_slot *el, int timeout_ms);
/// Sets the polling type: read/write/other. O_POLL_READ | O_POLL_WRITE | O_POLL_OTHER
void onion_poller_slot_set_type(onion_poller_slot *el, onion_poller_slot_type_e type);

/// Create a new poller
onion_poller *onion_poller_new(int aprox_n);
/// Frees the poller. It first stops it.
void onion_poller_free(onion_poller *);

/// Sets the max events per thread queue size.
void onion_poller_set_queue_size_per_thread(onion_poller *poller, size_t count);

/// Adds a slot to the poller
int onion_poller_add(onion_poller *poller, onion_poller_slot *el);
/// Gets the poller to do some modifications as change shutdown
onion_poller_slot *onion_poller_get(onion_poller *poller, int fd);
/// Removes a fd from the poller
int onion_poller_remove(onion_poller *poller, int fd);

/// Do the polling. If on several threads, this is done in every thread.
void onion_poller_poll(onion_poller *);
/// Stops the polling. This only marks the flag, and should be cancelled with pthread_cancel.
void onion_poller_stop(onion_poller *);

#ifdef __cplusplus
}
#endif


#endif
