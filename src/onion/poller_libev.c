/**
  Onion HTTP server library
  Copyright (C) 2010-2018 David Moreno Montero and others

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

#include <ev.h>
#include <stdlib.h>
#include <semaphore.h>

#include "poller.h"
#include "log.h"
#include "low.h"

struct onion_poller_t {
  struct ev_loop *loop;
  sem_t sem;
  volatile int stop;
};

struct onion_poller_slot_t {
  int fd;
  int timeout;
  int type;
  ev_io ev;
  void *data;
  int (*f) (void *);
  void *shutdown_data;
  void (*shutdown) (void *);
  onion_poller *poller;
};

/// Create a new slot for the poller
onion_poller_slot *onion_poller_slot_new(int fd, int (*f) (void *), void *data) {
  onion_poller_slot *ret = onion_low_calloc(1, sizeof(onion_poller_slot));
  ret->fd = fd;
  ret->f = f;
  ret->data = data;
  ret->type = EV_READ | EV_WRITE;

  return ret;
}

/// Cleans a poller slot. Do not call if already on the poller (onion_poller_add). Use onion_poller_remove instead.
void onion_poller_slot_free(onion_poller_slot * el) {
  if (el->poller)
    ev_io_stop(el->poller->loop, &el->ev);
  if (el->shutdown)
    el->shutdown(el->shutdown_data);
}

/// Sets the shutdown function for this poller slot
void onion_poller_slot_set_shutdown(onion_poller_slot * el,
                                    void (*shutdown) (void *), void *data) {
  el->shutdown = shutdown;
  el->shutdown_data = data;
}

/// Sets the timeout for this slot. Current implementation takes ms, but then it rounds to seconds.
void onion_poller_slot_set_timeout(onion_poller_slot * el, int timeout_ms) {
  el->timeout = timeout_ms;
}

/// Sets the polling type: read/write/other. O_POLL_READ | O_POLL_WRITE | O_POLL_OTHER
void onion_poller_slot_set_type(onion_poller_slot * el, onion_poller_slot_type_e type) {
  el->type = 0;
  if (type & O_POLL_READ)
    el->type |= EV_READ;
  if (type & O_POLL_WRITE)
    el->type |= EV_WRITE;
}

/// Create a new poller
onion_poller *onion_poller_new(int aprox_n) {
  onion_poller *ret = onion_low_calloc(1, sizeof(onion_poller));
  ret->loop = ev_default_loop(0);
  sem_init(&ret->sem, 0, 1);
  return ret;
}

/// Frees the poller. It first stops it.
void onion_poller_free(onion_poller * p) {
  onion_low_free(p);
}

static void event_callback(struct ev_loop *loop, ev_io * w, int revents) {
  onion_poller_slot *s = w->data;
  int res = s->f(s->data);
  if (res < 0) {
    onion_poller_slot_free(s);
  }
}

/// Adds a slot to the poller
int onion_poller_add(onion_poller * poller, onion_poller_slot * el) {
  el->poller = poller;

  // normally would use ev_io_init bellow, but gcc on F18+ give a
  // "dereferencing type-punned pointer will break strict-aliasing rules" error
  // So the macro must be expanded and this is what we get. In the future this may
  // give bugs on libevent if this changes.

  //ev_io_init(&el->ev, event_callback, el->fd, el->type);
  // expands to ev_init + ev_io_set. ev_init expand more or less as bellow

  //ev_init(&el->ev, event_callback);
  {
    ev_watcher *ew = (void *)(&el->ev); // ew must exit to prevent the before mentioned error.
    ew->active = 0;
    ew->pending = 0;
    ew->priority = 0;
    el->ev.cb = event_callback;
  }

  ev_io_set(&el->ev, el->fd, el->type);

  el->ev.data = el;
//      if (el->timeout>0){
//              event_add(el->ev, &tv);
//      }
//      else{
//              event_add(el->ev, NULL);
//      }
  ev_io_start(poller->loop, &el->ev);
  return 1;
}

/// Removes a fd from the poller
int onion_poller_remove(onion_poller * poller, int fd) {
  ONION_ERROR("FIXME!! not removing fd %d", fd);
  return -1;
}

/// Gets the poller to do some modifications as change shutdown
onion_poller_slot *onion_poller_get(onion_poller * poller, int fd) {
  ONION_ERROR("Not implemented! Use epoll poller.");
  return NULL;
}

/// Do the polling. If on several threads, this is done in every thread.
void onion_poller_poll(onion_poller * poller) {
  ev_default_fork();
  ev_loop_fork(poller->loop);

  poller->stop = 0;
  while (!poller->stop) {
    sem_wait(&poller->sem);
    ev_run(poller->loop, EVLOOP_ONESHOT);
    sem_post(&poller->sem);
  }
}

/// Stops the polling. This only marks the flag, and should be cancelled with pthread_cancel.
void onion_poller_stop(onion_poller * poller) {
  poller->stop = 1;
  ev_break(poller->loop, EVBREAK_ALL);
}

// Not implemented for libev
void onion_poller_set_queue_size_per_thread(onion_poller * poller, size_t count) {
  ONION_WARNING
      ("onion_poller_queue_size_per_thread only used with epoll polling, not libev.");
}
