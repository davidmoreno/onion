/*
	Onion HTTP server library
	Copyright (C) 2010-2014 David Moreno Montero and othes

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

#include <errno.h>
#include <sys/epoll.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>
#ifdef __DEBUG__
#include <execinfo.h>
#endif
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

#include "log.h"
#include "types.h"
#include "poller.h"
#include "low.h"
#include <sys/socket.h>
#include <sys/eventfd.h>
#include <fcntl.h>

#ifdef HAVE_PTHREADS
# ifndef __USE_UNIX98
#  define __USE_UNIX98
#  include <pthread.h>
#  undef __USE_UNIX98
# else
#  include <pthread.h>
# endif
#else  // if no pthreads, ignore locks.
# define pthread_mutex_init(...)
# define pthread_mutex_lock(...)
# define pthread_mutex_trylock(...) (0)
# define pthread_mutex_unlock(...)
#endif


struct onion_poller_t{
	int fd;
	int eventfd; ///< fd to signal internal changes on poller.
	int n;
  char stop;
#ifdef HAVE_PTHREADS
	pthread_mutex_t mutex;
#endif
	int npollers;

	onion_poller_slot **slots;
	int slot_count;
	int slot_max; // Max current position, small optimization so that search is not O(slot_count)
	uint64_t slot_id; // Increasing number for each new poller.
};

/// Each element of the poll
/// @private
struct onion_poller_slot_t{
	uint64_t id; // Serial id per poller.
	int fd;
	int (*f)(void*);
	void *data;
	int type;

	void (*shutdown)(void*);
	void *shutdown_data;

	time_t timeout;
	time_t timeout_limit; ///< Limit in seconds for use with time function.
};

static onion_poller_slot **onion_poller_get_by_id(onion_poller *poller, uint64_t id);


/**
 * @short Creates a new slot for the poller, for input data to be ready.
 * @memberof onion_poller_slot_t
 *
 * @param fd File descriptor to watch
 * @param f Function to call when data is ready. If function returns <0, the slot will be removed.
 * @param data Data to pass to the function.
 *
 * @returns A new poller slot, ready to be added (onion_poller_add) or modified (onion_poller_slot_set_shutdown, onion_poller_slot_set_timeout).
 */
onion_poller_slot *onion_poller_slot_new(int fd, int (*f)(void*), void *data){
	if (fd<0){
		ONION_ERROR("Trying to add an invalid file descriptor to the poller. Please check.");
		return NULL;
	}
	onion_poller_slot *el=(onion_poller_slot*)onion_low_calloc(1, sizeof(onion_poller_slot));
	el->fd=fd;
	el->f=f;
	el->data=data;
	el->timeout=-1;
	el->timeout_limit=INT_MAX;
	el->type=EPOLLIN | EPOLLHUP | EPOLLONESHOT | EPOLLHUP;

	ONION_DEBUG0("New slot fd %d, %p", el->fd, el);

	return el;
}

/**
 * @short Free the data for the given slot, calling shutdown if any.
 * @memberof onion_poller_slot_t
 */
void onion_poller_slot_free(onion_poller_slot *el){
	ONION_DEBUG0("Free slot fd %d, %p", el->fd, el);
	if (el->shutdown)
		el->shutdown(el->shutdown_data);
	onion_low_free(el);
}

/**
 * @short Sets a function to be called when the slot is removed, for example because the file is closed.
 * @memberof onion_poller_slot_t
 *
 * @param el slot
 * @param sd Function to call
 * @param data Parameter for the function
 */
void onion_poller_slot_set_shutdown(onion_poller_slot *el, void (*sd)(void*), void *data){
	el->shutdown=sd;
	el->shutdown_data=data;
}

/**
 * @short Sets the timeout for the slot
 *
 * The timeout is passed in ms, but due to current implementation it is rounded to seconds. The interface stays, and hopefully
 * in the future the behaviour will change.
 * @memberof onion_poller_slot_t
 *
 * @param el Slot to modify
 * @param timeout Time in milliseconds that this file can be waiting.
 */
void onion_poller_slot_set_timeout(onion_poller_slot *el, int timeout){
	el->timeout=timeout/1000; // I dont have that resolution.
	el->timeout_limit=time(NULL)+el->timeout;
	//ONION_DEBUG0("Set timeout to %d, %d s", el->timeout_limit, el->timeout);
}

void onion_poller_slot_set_type(onion_poller_slot *el, onion_poller_slot_type_e type){
	el->type=EPOLLONESHOT | EPOLLHUP | EPOLLRDHUP;
	if (type&O_POLL_READ)
		el->type|=EPOLLIN;
	if (type&O_POLL_WRITE)
		el->type|=EPOLLOUT;
	if (type&O_POLL_OTHER)
		el->type|=EPOLLERR|EPOLLPRI;
	ONION_DEBUG0("Setting type to %d, %d", el->fd, el->type);
}

static int onion_poller_stop_helper(void *p){
	onion_poller_stop(p);
  return 1;
}

#ifndef EFD_CLOEXEC
#define EFD_CLOEXEC 0
#endif

/**
 * @short Returns a poller object that helps polling on sockets and files
 * @memberof onion_poller_t
 *
 * This poller is implemented through epoll, but other implementations are possible
 *
 * n is the max number of connectins to manage. It affects memory, around 80 bytes
 *   per connection.
 */
onion_poller *onion_poller_new(int n){
	onion_poller *p=onion_low_malloc(sizeof(onion_poller));
	p->fd=epoll_create1(EPOLL_CLOEXEC);
	if (p->fd < 0){
		ONION_ERROR("Error creating the poller. %s", strerror(errno));
		onion_low_free(p);
		return NULL;
	}
	p->eventfd=eventfd(0,EFD_CLOEXEC | EFD_NONBLOCK);
#if EFD_CLOEXEC == 0
  fcntl(p->eventfd,F_SETFD,FD_CLOEXEC);
#endif
	p->n=0;
  p->stop=0;
	p->slot_count=(n > 128) ? n : 128; // min slot size
	p->slots=onion_low_malloc(sizeof(onion_poller_slot*)*p->slot_count);
	p->slot_max=0;
	p->slot_id=1;

#ifdef HAVE_PTHREADS
  ONION_DEBUG("Init thread stuff for poll. Eventfd at %d", p->eventfd);
  p->npollers=0;
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&p->mutex, &attr);
  pthread_mutexattr_destroy(&attr);
#endif

  onion_poller_slot *ev=onion_poller_slot_new(p->eventfd,onion_poller_stop_helper,p);
  onion_poller_add(p,ev);

	return p;
}

/// @memberof onion_poller_t
void onion_poller_free(onion_poller *p){
	ONION_DEBUG("Free onion poller: %d waiting", p->n);
	p->stop=1;
	close(p->fd);
	// Wait until all pollers exit.

	if (pthread_mutex_trylock(&p->mutex)>0){
		ONION_WARNING("When cleaning the poller object, some poller is still active; not freeing memory");
	}
	else{
		for(int i=0;i<p->slot_max;i++){
			if (p->slots[i])
				onion_poller_slot_free(p->slots[i]);
		}
		onion_low_free(p->slots);
		pthread_mutex_unlock(&p->mutex);
		onion_low_free(p);
	}
	ONION_DEBUG0("Done");
}

/**
 * @short Adds a file descriptor to poll.
 * @memberof onion_poller_t
 *
 * When new data is available (read/write/event) the given function
 * is called with that data.
 *
 * Once the slot is added is not safe anymore to set data on it, unless its detached from the epoll. (see EPOLLONESHOT)
 */
int onion_poller_add(onion_poller *poller, onion_poller_slot *el){

	pthread_mutex_lock(&poller->mutex);
	// Find an empty spot for the poller
	int i;
	for (i=0;i<poller->slot_max;i++){
		if (!poller->slots[i])
			break;
	}
	if (i==poller->slot_count){
		ONION_DEBUG("Poller descriptor space exhausted. Increasing."); // FIXME, also reduce!
		i=poller->slot_count;
		if (poller->slot_count<4096)
			poller->slot_count*=2;
		else
			poller->slot_count+=256;
		poller->slots=onion_low_realloc(poller->slots, sizeof(onion_poller_slot*)*poller->slot_count);
	}
	assert(i<poller->slot_count);
	if (i>=poller->slot_max)
		poller->slot_max=i+1;

	// Place it
	poller->slots[i]=el;
	el->id=poller->slot_id++;
	poller->n++;

	ONION_DEBUG0("Adding fd %d for polling (%d), id %d, slot %d/%d", el->fd, poller->n, el->id, i, poller->slot_max);
	pthread_mutex_unlock(&poller->mutex);

	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.events=el->type;
	ev.data.u64=el->id; // We will look for it by id. If not there not being used anymore
	if (epoll_ctl(poller->fd, EPOLL_CTL_ADD, el->fd, &ev) < 0){
		ONION_ERROR("Error add descriptor to listen to. %s", strerror(errno));
		return 1;
	}
	return 1;
}


/**
 * @short Removes a file descriptor, and all related callbacks from the listening queue and frees the slot
 * @memberof onion_poller_t
 *
 * @returns -1 on error
 */
int onion_poller_remove_and_free_by_id(onion_poller *poller, uint64_t id){
	pthread_mutex_lock(&poller->mutex);
	onion_poller_slot **slot=onion_poller_get_by_id(poller, id);
	if (slot){
		onion_poller_slot *cur=*slot;
		ONION_DEBUG0("Remove id %d, fd %d from slot list (%d left)", cur->fd, cur->id, poller->n);

		if (epoll_ctl(poller->fd, EPOLL_CTL_DEL, cur->fd, NULL) < 0){
			if (errno!=ENOENT && errno!=EBADF){
				ONION_ERROR("Error remove descriptor to listen to. %s (%d)", strerror(errno), errno);
			}
		}

		*slot=NULL;
		if (poller->slot_max-1 == cur->id){
			for (int i=poller->slot_max-2;i>0;i--){ // slot_max is always one ahead, as I remove myself, start looking 2 before.
				if (poller->slots[i]){
					poller->slot_max=i+1;
					break;
				}
			}
		}

		onion_poller_slot_free( cur );

		pthread_mutex_unlock(&poller->mutex);
		return 0;
	}
	pthread_mutex_unlock(&poller->mutex);
	return -1;
}

/**
 * @short Removes a file descriptor, and all related callbacks from the listening queue
 * @memberof onion_poller_t
 *
 * @returns the poller slot
 */
onion_poller_slot *onion_poller_remove(onion_poller *poller, int fd){
	if (epoll_ctl(poller->fd, EPOLL_CTL_DEL, fd, NULL) < 0){
		if (errno!=ENOENT && errno!=EBADF){
			ONION_ERROR("Error remove descriptor to listen to. %s (%d)", strerror(errno), errno);
		}
	}

	ONION_DEBUG0("Remove fd %d from slot list (%d left)", fd, poller->n);
	pthread_mutex_lock(&poller->mutex);

	onion_poller_slot **slot=poller->slots;
	int newmax=0;
	for (int i=0;i<poller->slot_max;i++){
		if (*slot){
			onion_poller_slot *cur=*slot;
			//ONION_DEBUG("Find fd %d==%d (at %d/%d)", fd, cur ? cur->fd : -1, i, poller->slot_max);
			if (cur->fd==fd){
				//ONION_DEBUG0("Found, removed");

				*slot=NULL;

				if (poller->slot_max-1 == i) // If I was the last, set to the one before me
					poller->slot_max=newmax+1;
				if (newmax==0){
					ONION_DEBUG0("Removed last slot, stoping poller");
					onion_poller_stop(poller);
				}
				pthread_mutex_unlock(&poller->mutex);
				//ONION_DEBUG("Done, newmax %d", poller->slot_max);
				return cur;
			}
			newmax=i;
		}
		slot++;
	}

	pthread_mutex_unlock(&poller->mutex);
	ONION_WARNING("Trying to remove unknown fd %d from poller", fd);
	return NULL;
}

/**
 * @short Gets the poller slot
 *
 * This might be used for long polling, removing the default deleter.
 */
onion_poller_slot *onion_poller_get(onion_poller *poller, int fd){
	pthread_mutex_lock(&poller->mutex);
	for(int i=0;i<poller->slot_max;i++){
		if (poller->slots[i] && poller->slots[i]->fd==fd)
			return poller->slots[i];
	}
	pthread_mutex_unlock(&poller->mutex);
	return NULL;
}

/**
 * @short Gets the poller slot
 *
 * This might be used for long polling, removing the default deleter.
 *
 * Returns the pointer to the location. Needs to be protected by the mutex
 * by parent.
 */
static onion_poller_slot **onion_poller_get_by_id(onion_poller *poller, uint64_t id){
	onion_poller_slot **I=poller->slots, **endI=poller->slots + poller->slot_max;
	for(;I!=endI;++I){
		onion_poller_slot *cur=*I;
		if (cur && cur->id==id)
			return I;
	}
	return NULL;
}

/**
 * @short Gets the next timeout
 *
 * On edge cases could get a wrong timeout, but always old or new, so its ok.
 *
 * List must be locked by caller.
 */
static int onion_poller_get_next_timeout(onion_poller *p){
	pthread_mutex_lock(&p->mutex);
	int timeout=INT_MAX; // Ok, no wakeup
	onion_poller_slot **slot=p->slots;
	for(int i=0;i<p->slot_max;i++){
		onion_poller_slot *cur=*slot;
		if (cur && cur->timeout_limit<timeout)
			timeout=cur->timeout_limit;
		slot++;
	}

	//ONION_DEBUG("Next wakeup in %d ms, at least", timeout);
	pthread_mutex_unlock(&p->mutex);
	return timeout;
}

void onion_poller_check_timeouts(onion_poller *p){
	//ONION_DEBUG0("Current time is %d, limit is %d, timeout is %d. Waited for %d seconds", ctime, maxtime, timeout, ctime_end-ctime);
	time_t time_now=time(NULL);

	pthread_mutex_lock(&p->mutex);
	onion_poller_slot **slot=p->slots;
	for(int i=0;i<p->slot_max;i++){
		onion_poller_slot *cur=*slot;
		if (cur && cur->timeout_limit <= time_now){
			ONION_DEBUG0("Timeout on fd %d, was %d (time_now %d)", cur->fd, cur->timeout_limit, time_now);
			cur->timeout_limit=INT_MAX;
			if (cur->shutdown){
				cur->shutdown(cur->shutdown_data);
				onion_poller_slot_set_shutdown(cur,NULL,NULL);
			}
			// closed, do not even try to call it.
			cur->f=NULL;
			cur->data=NULL;
		}
		slot++;
	}
	pthread_mutex_unlock(&p->mutex);
}

// Do all necessary to serve one poller wake up
static void onion_poller_serve_one(onion_poller *p, onion_poller_slot *el, struct epoll_event *event){
	ONION_DEBUG0("Use slot fd %d, %X, %p", el->fd, event->events, el);

	int n=-1;
	if (event->events&(EPOLLRDHUP | EPOLLHUP)){
		n=-1;
	}
	else{
#ifdef __DEBUG0__
    char **bs=backtrace_symbols((void * const *)&el->f, 1);
    ONION_DEBUG0("Calling handler: %s (%d)",bs[0], el->fd);
    free(bs); /* This cannot be onion_low_free since from
     backtrace_symbols. */
#endif
/* Sometimes, el->f happens to be null. We want to remove this
 	polling in that weird case. */
		if (el->f)
		  n= el->f(el->data);
		else
		  n= -1;

		time_t ctime=time(NULL);
		if (el->timeout>0){
			pthread_mutex_lock(&p->mutex);
			el->timeout_limit=ctime+el->timeout;
			pthread_mutex_unlock(&p->mutex);
		}
	}
	if (n<0){
		ONION_DEBUG0("Remove fd %d as n<0", el->fd);
		onion_poller_remove_and_free_by_id(p, event->data.u64);
	}
	else{
		ONION_DEBUG0("Re setting poller %d", el->fd);
		event->events=el->type;
		if (p->fd>=0){
			el->id=event->data.u64; // Recover id, to be searchcable again.
			int e=epoll_ctl(p->fd, EPOLL_CTL_MOD, el->fd, event);
			if (e<0){
				onion_poller_remove_and_free_by_id(p, event->data.u64);
				ONION_ERROR("Error resetting poller. Remove it. %s", strerror(errno));
			}
		}
	}
}

/**
 * @short Do the event polling.
 * @memberof onion_poller_t
 *
 * It loops over polling. To exit polling call onion_poller_stop().
 *
 * If no fd to poll, returns.
 */
void onion_poller_poll(onion_poller *p){
	struct epoll_event event[1];
	ONION_DEBUG("Start polling");
	p->stop=0;

	pthread_mutex_lock(&p->mutex);
	p->npollers++;
	ONION_DEBUG0("Npoller %d/%d", p->npollers, p->n);
	pthread_mutex_unlock(&p->mutex);

	int maxtime;
	int timeout;
	while (!p->stop){
		maxtime=onion_poller_get_next_timeout(p);

		time_t ctime=time(NULL);
		timeout=maxtime-ctime;
		if (timeout>3600)
			timeout=3600000;
		else
			timeout*=1000;
		//ONION_DEBUG0("Wait for %d ms", timeout);
		int nfds = epoll_wait(p->fd, event, 1, timeout);

		if (nfds<=0){ // This is normally closed p->fd, but could be just spurius epoll nfds==0
			//ONION_DEBUG("Some error happened"); // Also spurious wakeups... gdb is to blame sometimes or any other.
			if(p->fd<0){
				ONION_DEBUG("Finishing the epoll as finished: %s", strerror(errno));
#ifdef HAVE_PTHREADS
				pthread_mutex_lock(&p->mutex);
				p->npollers--;
				pthread_mutex_unlock(&p->mutex);
#endif
				return;
			}
			continue; // The rest assumes one to read.
		}
		ONION_DEBUG0("%d ready to read", nfds);
		ONION_DEBUG0("Ready to read id %ld", event->data.u64);

		pthread_mutex_lock(&p->mutex);
		uint64_t id=event->data.u64;
		onion_poller_slot **slot=onion_poller_get_by_id(p, id);
		onion_poller_slot *el=NULL;
		if (slot){
			el=*slot;
			el->id=-1;
			el->timeout_limit=INT_MAX;
		}

		onion_poller_check_timeouts(p);
		pthread_mutex_unlock(&p->mutex);
		// This may happen if fd prepared in thread A, blocks between the
		// epoll wake and here, thread B timeouts the fd, schedules for
		// delete, thread C deletes it, and finally thread A continues.
		// So here, to hijack thread C from finding it, I nullify temporarily
		// the id. Will set it properly when reset the slot.
		// If closed should signal at epoll ctl reset.
		if (!el){
			ONION_DEBUG0("Not found id %d, maybe gone on another thread.", id);
			continue;
		}

		onion_poller_serve_one(p, el, event);
	}

	ONION_DEBUG("Finished polling fds");
	pthread_mutex_lock(&p->mutex);
	p->npollers--;
	ONION_DEBUG0("Npollers %d", p->npollers);
	pthread_mutex_unlock(&p->mutex);
}

/**
 * @short Marks the poller to stop ASAP
 * @memberof onion_poller_t
 */
void onion_poller_stop(onion_poller *p){
  ONION_DEBUG("Stopping poller");
  p->stop=1;
  char data[8]={0,0,0,0, 0,0,0,1};
  int __attribute__((unused)) r=read(p->eventfd, data, 8); // Flush eventfd data, discard data

#ifdef HAVE_PTHREADS
	pthread_mutex_lock(&p->mutex);
  int n=p->npollers;
	pthread_mutex_unlock(&p->mutex);

  if (n>0){
		int w=write(p->eventfd,data,8); // Tell another thread to exit
		if (w<0){
			ONION_ERROR("Error signaling poller to stop!");
		}
	}
	else
		ONION_DEBUG("Poller stopped");
#endif
}
