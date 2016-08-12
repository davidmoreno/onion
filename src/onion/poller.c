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

#include <errno.h>
#include <sys/epoll.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>
#include <sys/timerfd.h>
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
#include <sys/resource.h>
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


/// @defgroup poller Poller. Queues files until ready to read/write and call a handler.

struct onion_poller_t{
	int fd;
	int eventfd; ///< fd to signal internal changes on poller.
	int timerfd; ///< fd to set up timeouts
	time_t current_timeout_limit; ///< Currently set limit in seconds

	int n;
  char stop;
#ifdef HAVE_PTHREADS
	pthread_mutex_t mutex;
	int npollers;
#endif

	onion_poller_slot *head;
};

/// Each element of the poll
/// @private
struct onion_poller_slot_t{
	int fd;
	int (*f)(void*);
	void *data;
	int type;

	void (*shutdown)(void*);
	void *shutdown_data;

	time_t timeout;
	time_t timeout_limit; ///< Limit in seconds for use with time function.

	onion_poller_slot *next;
};

// Max number of polls, normally just 1024 as set by `ulimit -n` (fd count).
static const int MAX_SLOTS=1000000;

static struct {
	onion_poller_slot empty_slot;
	onion_poller_slot *slots;
	rlim_t max_slots;
	int refcount;
} onion_poller_static = {{}, NULL, 0, 0};

/// Initializes static data. Init at onion_poller_new, deinit at _free.
static void onion_poller_static_init(){
	int16_t prevcount = __sync_fetch_and_add(&onion_poller_static.refcount, 1);
	if (prevcount!=0) // Only init first time
		return;

	memset(&onion_poller_static.empty_slot, 0, sizeof(onion_poller_static.empty_slot));

	struct rlimit rlim;
	if (getrlimit(RLIMIT_NOFILE, &rlim)) {
		ONION_ERROR("getrlimit: %s", strerror(errno));
		return;
	}
	onion_poller_static.max_slots = rlim.rlim_cur;
	if (onion_poller_static.max_slots > MAX_SLOTS)
		onion_poller_static.max_slots = MAX_SLOTS;
	onion_poller_static.slots = (onion_poller_slot *)onion_low_calloc(onion_poller_static.max_slots, sizeof(onion_poller_slot));
}

/// Deinits static data at onion_poller.
static void onion_poller_static_deinit(){
	int16_t nextcount = __sync_sub_and_fetch(&onion_poller_static.refcount, 1);
	if (nextcount!=0)
		return;

	onion_low_free(onion_poller_static.slots);
}

/// Wrapper around clock_gettime(CLOCK_MONOTONIC_RAW, &t)
/// It returns an unspecefified monotonic time in seconds. used for intervals.
static time_t onion_time(){
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC_RAW, &t);
	return t.tv_sec;
}

/**
 * @short Creates a new slot for the poller, for input data to be ready.
 * @memberof onion_poller_slot_t
 * @ingroup poller
  *
 * @param fd File descriptor to watch
 * @param f Function to call when data is ready. If function returns <0, the slot will be removed.
 * @param data Data to pass to the function.
 *
 * @returns A new poller slot, ready to be added (onion_poller_add) or modified (onion_poller_slot_set_shutdown, onion_poller_slot_set_timeout).
 */
onion_poller_slot *onion_poller_slot_new(int fd, int (*f)(void*), void *data){
	if (fd<0||fd>=onion_poller_static.max_slots){
		ONION_ERROR("Trying to add an invalid file descriptor to the poller (%d). Please check. Must be (0-%d)", fd, onion_poller_static.max_slots);
		return NULL;
	}
	onion_poller_slot *el=&onion_poller_static.slots[fd];
	*el=onion_poller_static.empty_slot;
	el->fd=fd;
	el->f=f;
	el->data=data;
	el->timeout=-1;
	el->timeout_limit=INT_MAX;
	el->type=EPOLLIN | EPOLLHUP | EPOLLONESHOT | EPOLLHUP;

	return el;
}

/**
 * @short Free the data for the given slot, calling shutdown if any.
 * @memberof onion_poller_slot_t
 * @ingroup poller
 */
void onion_poller_slot_free(onion_poller_slot *el){
/* Cannot zeroize the slot here because it's still needed for el->shutdown(),
 * and it can be reused by another thread when ->shutdown() calls close(). */
	if (el->shutdown)
		el->shutdown(el->shutdown_data);
}

/**
 * @short Sets a function to be called when the slot is removed, for example because the file is closed.
 * @memberof onion_poller_slot_t
 * @ingroup poller
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
 * @ingroup poller
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
	el->timeout_limit=onion_time()+el->timeout;
	ONION_DEBUG0("Set timeout to %d, %d s", el->timeout_limit, el->timeout);
}

/// Sets the type of the poller
/// @ingroup poller
void onion_poller_slot_set_type(onion_poller_slot *el, onion_poller_slot_type_e type){
	el->type=EPOLLONESHOT | EPOLLHUP;
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

static time_t onion_poller_last_time=0;

static int onion_poller_timer(void *p_);

static void onion_poller_timer_check(onion_poller *p, time_t timelimit){
	// If maybe timeout limit (p->current_timeout_limit is used atomically)
	if (timelimit < p->current_timeout_limit){
		// Recalculate with proper mutexes.
		onion_poller_timer(p);
	}
}

static int onion_poller_timer(void *p_){
	onion_poller *p=p_;
	//ONION_DEBUG("Tick");

	// It seems its ok to ignore expirations.
	//int64_t count=0;
	//if (read(p->timerfd, &count, sizeof(count))>0){ // Triggered.. not really interested
	//}

	time_t ctime = onion_time();
	time_t next_timeout=ctime + (24*60*60); // At least once per day

	// Do this only once per second max.
	if (ctime != onion_poller_last_time) {
		onion_poller_last_time = ctime;
		pthread_mutex_lock(&p->mutex);
		onion_poller_slot *next=p->head;
		while (next){
			onion_poller_slot *cur=next;
			next=next->next;
			if (cur->timeout_limit <= ctime){
				ONION_DEBUG("Timeout on %d, was %d (ctime %d)", cur->fd, cur->timeout_limit, ctime);
				cur->timeout_limit=INT_MAX;
				shutdown(cur->fd, SHUT_RD);
			}
			else if (cur->timeout_limit<next_timeout){
				next_timeout=cur->timeout_limit;
			}
		}
		pthread_mutex_unlock(&p->mutex);
	}

	// Atomic store.
	// Try until set if <= next_timeout.
	time_t origval;
	time_t oldval;
	do{ // Spin until change is acceptable.
		origval=p->current_timeout_limit;
		oldval=__sync_val_compare_and_swap(&p->current_timeout_limit, origval, next_timeout);
	}while ( // Repeat if
		origval!=oldval // different, so not changed because somebody else changed the value
		&& oldval>next_timeout // and current timer is for later than my timeout
	); // Ths is when another thread changed if for a later timeout, so I must insist on mine

	//ONION_DEBUG("Next timeout in %d seconds", p->current_timeout_limit - ctime);
	// rearm
	struct itimerspec next={{0,0},{next_timeout-ctime,0}};
	timerfd_settime(p->timerfd, 0, &next, NULL );

	return 1;
}

#ifndef EFD_CLOEXEC
#define EFD_CLOEXEC 0
#endif

/**
 * @short Returns a poller object that helps polling on sockets and files
 * @memberof onion_poller_t
 * @ingroup poller
 *
 * This poller is implemented through epoll, but other implementations are possible
 *
 */
onion_poller *onion_poller_new(int n){
	onion_poller *p=onion_low_calloc(1, sizeof(onion_poller));
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
	p->head=NULL;
	p->n=0;
  p->stop=0;

#ifdef HAVE_PTHREADS
  ONION_DEBUG("Init thread stuff for poll. Eventfd at %d", p->eventfd);
  p->npollers=0;
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&p->mutex, &attr);
  pthread_mutexattr_destroy(&attr);
#endif

	onion_poller_static_init();


  onion_poller_slot *ev=onion_poller_slot_new(p->eventfd,onion_poller_stop_helper,p);
  onion_poller_add(p,ev);

	p->timerfd=timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
	ev=onion_poller_slot_new(p->timerfd,&onion_poller_timer,p);
	onion_poller_add(p,ev);
	onion_poller_timer(p); // Force first timeout

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
		onion_poller_slot *next=p->head;
		while (next){
			onion_poller_slot *tnext=next->next;
			if (next->shutdown)
				next->shutdown(next->shutdown_data);
			next=tnext;
		}
		pthread_mutex_unlock(&p->mutex);

		if (p->eventfd>=0)
			close(p->eventfd);
		if (p->timerfd>=0)
			close(p->timerfd);

		onion_poller_static_deinit();

		onion_low_free(p);
	}
	ONION_DEBUG0("Done");
}

/**
 * @short Adds a file descriptor to poll.
 * @memberof onion_poller_t
 * @ingroup poller
 *
 * When new data is available (read/write/event) the given function
 * is called with that data.
 *
 * Once the slot is added is not safe anymore to set data on it, unless its detached from the epoll. (see EPOLLONESHOT)
 */
int onion_poller_add(onion_poller *poller, onion_poller_slot *el){
	ONION_DEBUG0("Adding fd %d for polling (%d)", el->fd, poller->n);

	pthread_mutex_lock(&poller->mutex);
	// I like head to be always the same, so I do some tricks to keep it. This is beacuse at head normally I have the listen fd, which is very accessed
	if (!poller->head)
		poller->head=el;
	else{
		el->next=poller->head->next;
		poller->head->next=el;
		poller->n++;
	}
	pthread_mutex_unlock(&poller->mutex);

	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.events=el->type;
	ev.data.ptr=el;
	if (epoll_ctl(poller->fd, EPOLL_CTL_ADD, el->fd, &ev) < 0){
		ONION_ERROR("Error add descriptor to listen to. %s", strerror(errno));
		return 1;
	}
	onion_poller_timer_check(poller, el->timeout_limit);

	return 1;
}

/**
 * @short Removes a file descriptor, and all related callbacks from the listening queue
 * @memberof onion_poller_t
 * @ingroup poller
 */
int onion_poller_remove(onion_poller *poller, int fd){
	if (epoll_ctl(poller->fd, EPOLL_CTL_DEL, fd, NULL) < 0){
		if (errno!=ENOENT && errno!=EBADF){
			ONION_ERROR("Error remove descriptor to listen to. %s (%d)", strerror(errno), errno);
		}
	}

	pthread_mutex_lock(&poller->mutex);
	ONION_DEBUG0("Trying to remove fd %d (%d)", fd, poller->n);
	onion_poller_slot *el=poller->head;
	if (el && el->fd==fd){
		ONION_DEBUG0("Removed from head %p", el);

		poller->head=el->next;
		pthread_mutex_unlock(&poller->mutex);

		onion_poller_slot_free(el);
		return 0;
	}
	while (el->next){
		if (el->next->fd==fd){
			ONION_DEBUG0("Removed from tail %p",el);
			onion_poller_slot *t=el->next;
			el->next=t->next;

      if (poller->head->next==NULL){ // This means only eventfd is here.
        ONION_DEBUG0("Removed last, stopping poll");
        onion_poller_stop(poller);
      }

			pthread_mutex_unlock(&poller->mutex);

			onion_poller_slot_free(t);
			return 0;
		}
		el=el->next;
	}
	pthread_mutex_unlock(&poller->mutex);
	ONION_WARNING("Trying to remove unknown fd from poller %d", fd);
	return 0;
}

/**
 * @short Gets the poller slot
 * @ingroup poller
 *
 * This might be used for long polling, removing the default deleter.
 */
onion_poller_slot *onion_poller_get(onion_poller *poller, int fd){
	onion_poller_slot *next=poller->head;
	while (next){
		if (next->fd==fd)
			return next;
		next=next->next;
	}
	return NULL;
}

// Max of events per loop. If not al consumed for next, so no prob.  right number uses less memory, and makes less calls.
static size_t onion_poller_max_events=1;

/**
 * @short Do the event polling.
 * @memberof onion_poller_t
 * @ingroup poller
 *
 * It loops over polling. To exit polling call onion_poller_stop().
 *
 * If no fd to poll, returns.
 */
void onion_poller_poll(onion_poller *p){
	struct epoll_event event[onion_poller_max_events];
	ONION_DEBUG("Start polling");
#ifdef HAVE_PTHREADS
	pthread_mutex_lock(&p->mutex);
	p->npollers++;
	p->stop=0;
	ONION_DEBUG0("Npollers %d. %d listenings %p", p->npollers, p->n, p->head);
	pthread_mutex_unlock(&p->mutex);
#else
	p->stop=0;
#endif
#ifdef HAVE_PTHREADS
	pthread_mutex_lock(&p->mutex);
	char stop = !p->stop && p->head;
	pthread_mutex_unlock(&p->mutex);
#else
	char stop = !p->stop && p->head;
#endif
	while (stop){
		int nfds = epoll_wait(p->fd, event, onion_poller_max_events, -1);

		if (nfds<0){ // This is normally closed p->fd
			//ONION_DEBUG("Some error happened"); // Also spurious wakeups... gdb is to blame sometimes or any other.
			if(p->fd<0 || !p->head){
				ONION_DEBUG("Finishing the epoll as finished: %s", strerror(errno));
#ifdef HAVE_PTHREADS
				pthread_mutex_lock(&p->mutex);
				p->npollers--;
				pthread_mutex_unlock(&p->mutex);
#endif
				return;
			}
		}
		int i;
		for (i=0;i<nfds;i++){
			onion_poller_slot *el=(onion_poller_slot*)event[i].data.ptr;
      if (!el)
        continue;
			// Call the callback
			//ONION_DEBUG("Calling callback for fd %d (%X %X)", el->fd, event[i].events);
			int n=-1;
			if (event[i].events&(EPOLLRDHUP | EPOLLHUP)){
				n=-1;
			}
			else{ // I also take care of the timeout, no timeout when on the handler, it should handle it itself.
				el->timeout_limit=INT_MAX;

#ifdef __DEBUG0__
        char **bs=backtrace_symbols((void * const *)&el->f, 1);
        ONION_DEBUG0("Calling handler: %s (%d)",bs[0], el->fd);
        onion_low_free(bs); /* This cannot be onion_low_free since from
		     backtrace_symbols. */
#endif
				n = el->f(el->data);

				if (el->timeout>0){
					el->timeout_limit=onion_time()+el->timeout;
					onion_poller_timer_check(p, el->timeout_limit);
				}
			}
			if (n<0){
				onion_poller_remove(p, el->fd);
			}
			else{
				ONION_DEBUG0("Re setting poller %d", el->fd);
				event[i].events=el->type;
				if (p->fd>=0){
					int e=epoll_ctl(p->fd, EPOLL_CTL_MOD, el->fd, &event[i]);
					if (e<0){
						ONION_ERROR("Error resetting poller, %s", strerror(errno));
					}
				}
			}
		}
#ifdef HAVE_PTHREADS
		pthread_mutex_lock(&p->mutex);
		stop = !p->stop && p->head;
		pthread_mutex_unlock(&p->mutex);
#else
		stop = !p->stop && p->head;
#endif
	}
	ONION_DEBUG("Finished polling fds");
#ifdef HAVE_PTHREADS
	pthread_mutex_lock(&p->mutex);
	p->npollers--;
	ONION_DEBUG0("Npollers %d", p->npollers);
	pthread_mutex_unlock(&p->mutex);
#endif
}

/**
 * @short Marks the poller to stop ASAP
 * @memberof onion_poller_t
 * @ingroup poller
 */
void onion_poller_stop(onion_poller *p){
  ONION_DEBUG("Stopping poller");
#ifdef HAVE_PTHREADS
	pthread_mutex_lock(&p->mutex);
  p->stop=1;
	pthread_mutex_unlock(&p->mutex);
#else
	p->stop=1;
#endif

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

/**
 * @short Sets the max events per thread queue size.
 * @ingroup poller
 *
 * This fine tune allows to change the queue of events per thread.
 *
 * In case of data contention a call to epoll_wait can return several ready
 * descriptors, which this value decides.
 *
 * If the value is high and some request processing is slow, the requests
 * after that one will have to wait until processed, which increases
 * latency. On the other hand, it requests are fast, using the queue is
 * faster than another call to epoll_wait.
 *
 * In non contention cases, where onion is mostly waiting for requests,
 * it makes no difference.
 *
 * WARNING! Just now there apears to be a bug on timeout management, and
 * changing this level increases the probability of hitting it.
 *
 * Default: 1.
 */
void onion_poller_set_queue_size_per_thread(onion_poller *poller, size_t count){
	assert(count>0);
	onion_poller_max_events=count;
}
