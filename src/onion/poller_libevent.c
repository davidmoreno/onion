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

#include <event2/event.h>
#include <event2/thread.h>
#include <stdlib.h>
#include <semaphore.h>

#include "poller.h"
#include "log.h"
#include "low.h"

struct onion_poller_t{
	struct event_base *base;
	sem_t sem;
	volatile int stop;
};

struct onion_poller_slot_t{
	int fd;
	int timeout;
	int type;
	struct event *ev;
	void *data;
	int (*f)(void*);
	void *shutdown_data;
	void (*shutdown)(void*);
};

typedef struct onion_poller_slot_t onion_poller_slot;

/// Create a new slot for the poller
onion_poller_slot *onion_poller_slot_new(int fd, int (*f)(void*), void *data){
	onion_poller_slot *ret=onion_low_calloc(1, sizeof(onion_poller_slot));
	ret->fd=fd;
	ret->f=f;
	ret->data=data;
	ret->type=EV_READ | EV_WRITE | EV_PERSIST;

	return ret;
}

/// Cleans a poller slot. Do not call if already on the poller (onion_poller_add). Use onion_poller_remove instead.
void onion_poller_slot_free(onion_poller_slot *el){
	if (el->ev)
		event_del(el->ev);
	if (el->shutdown)
		el->shutdown(el->shutdown_data);
}
/// Sets the shutdown function for this poller slot
void onion_poller_slot_set_shutdown(onion_poller_slot *el, void (*shutdown)(void*), void *data){
	el->shutdown=shutdown;
	el->shutdown_data=data;
}
/// Sets the timeout for this slot. Current implementation takes ms, but then it rounds to seconds.
void onion_poller_slot_set_timeout(onion_poller_slot *el, int timeout_ms){
	el->timeout=timeout_ms;
}
/// Sets the polling type: read/write/other. O_POLL_READ | O_POLL_WRITE | O_POLL_OTHER
void onion_poller_slot_set_type(onion_poller_slot *el, int type){
	el->type=EV_PERSIST;
	if (type&O_POLL_READ)
		el->type|=EV_READ;
	if (type&O_POLL_WRITE)
		el->type|=EV_WRITE;
}

/// Create a new poller
onion_poller *onion_poller_new(int aprox_n){
	evthread_use_pthreads();

	onion_poller *ret=onion_low_calloc(1,sizeof(onion_poller));
	ret->base=event_base_new();
	sem_init(&ret->sem, 0, 1);
	return ret;
}

/// Frees the poller. It first stops it.
void onion_poller_free(onion_poller *p){
	event_base_free(p->base);
	onion_low_free(p);
}

static void event_callback(evutil_socket_t fd, short evtype, void *e){
	onion_poller_slot *s=e;
	int res=s->f(s->data);
	if (res<0){
		onion_poller_slot_free(s);
	}
}

/// Adds a slot to the poller
int onion_poller_add(onion_poller *poller, onion_poller_slot *el){
	el->ev=event_new(poller->base, el->fd, el->type, event_callback, el);
	if (el->timeout>0){
		struct timeval tv;
		tv.tv_sec=el->timeout/1000;
		tv.tv_usec=1000*(el->timeout%1000);
		event_add(el->ev, &tv);
	}
	else{
		event_add(el->ev, NULL);
	}
	return 1;
}

/// Removes a fd from the poller
int onion_poller_remove(onion_poller *poller, int fd){
	ONION_ERROR("FIXME!! not removing fd %d", fd);
	return -1;
}

/// Gets the poller to do some modifications as change shutdown
onion_poller_slot *onion_poller_get(onion_poller *poller, int fd){
	ONION_ERROR("Not implemented! Use epoll poller.");
	return NULL;
}


/// Do the polling. If on several threads, this is done in every thread.
void onion_poller_poll(onion_poller *poller){
	poller->stop=0;
	while(!poller->stop){
		sem_wait(&poller->sem);
		event_base_loop(poller->base,EVLOOP_ONCE);
		sem_post(&poller->sem);
	}
}
/// Stops the polling. This only marks the flag, and should be cancelled with pthread_cancel.
void onion_poller_stop(onion_poller *poller){
	poller->stop=1;
	event_base_loopbreak(poller->base);
}

// Not implemented for libev
void onion_poller_set_queue_size_per_thread(onion_poller *poller, size_t count){
	ONION_WARNING("onion_poller_queue_size_per_thread only used with epoll polling, not libev.");
}
