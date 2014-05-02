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

#include <malloc.h>
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
	onion_poller_slot *el=(onion_poller_slot*)calloc(1, sizeof(onion_poller_slot));
	el->fd=fd;
	el->f=f;
	el->data=data;
	el->timeout=-1;
	el->timeout_limit=INT_MAX;
	el->type=EPOLLIN | EPOLLHUP | EPOLLONESHOT;
	
	return el;
}

/**
 * @short Free the data for the given slot, calling shutdown if any.
 * @memberof onion_poller_slot_t
 */
void onion_poller_slot_free(onion_poller_slot *el){
	if (el->shutdown)
		el->shutdown(el->shutdown_data);
	free(el);
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
	ONION_DEBUG0("Set timeout to %d, %d s", el->timeout_limit, el->timeout);
}

void onion_poller_slot_set_type(onion_poller_slot *el, int type){
	el->type=EPOLLONESHOT;
	if (type&O_POLL_READ)
		el->type|=EPOLLIN;
	if (type&O_POLL_WRITE)
		el->type|=EPOLLOUT;
	if (type&O_POLL_OTHER)
		el->type|=EPOLLERR|EPOLLHUP|EPOLLPRI;
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
 */
onion_poller *onion_poller_new(int n){
	onion_poller *p=malloc(sizeof(onion_poller));
	p->fd=epoll_create1(EPOLL_CLOEXEC);
	if (p->fd < 0){
		ONION_ERROR("Error creating the poller. %s", strerror(errno));
		free(p);
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
		onion_poller_slot *next=p->head;
		while (next){
			onion_poller_slot *tnext=next->next;
			if (next->shutdown)
				next->shutdown(next->shutdown_data);
			free(next);
			next=tnext;
		}
		pthread_mutex_unlock(&p->mutex);
		free(p);
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
	return 1;
}

/**
 * @short Removes a file descriptor, and all related callbacks from the listening queue
 * @memberof onion_poller_t
 */
int onion_poller_remove(onion_poller *poller, int fd){
	if (epoll_ctl(poller->fd, EPOLL_CTL_DEL, fd, NULL) < 0){
		ONION_ERROR("Error remove descriptor to listen to. %s", strerror(errno));
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
 * @short Gets the next timeout
 * 
 * On edge cases could get a wrong timeout, but always old or new, so its ok.
 * 
 * List must be locked by caller.
 */
static int onion_poller_get_next_timeout(onion_poller *p){
	onion_poller_slot *next;
	int timeout=INT_MAX; // Ok, minimum wakeup , once per hour.
	next=p->head;
	while(next){
		//ONION_DEBUG("Check %d %d %d", timeout, next->timeout, next->delta_timeout);
		if (next->timeout_limit<timeout)
			timeout=next->timeout_limit;
		
		next=next->next;
	}
	
	//ONION_DEBUG("Next wakeup in %d ms, at least", timeout);
	return timeout;
}

// Max of events per loop. If not al consumed for next, so no prob.  right number uses less memory, and makes less calls.
#define MAX_EVENTS 10

/**
 * @short Do the event polling.
 * @memberof onion_poller_t
 *
 * It loops over polling. To exit polling call onion_poller_stop().
 * 
 * If no fd to poll, returns.
 */
void onion_poller_poll(onion_poller *p){
	struct epoll_event event[MAX_EVENTS];
	ONION_DEBUG("Start polling");
	p->stop=0;
#ifdef HAVE_PTHREADS
	pthread_mutex_lock(&p->mutex);
	p->npollers++;
	ONION_DEBUG0("Npollers %d. %d listenings %p", p->npollers, p->n, p->head);
	pthread_mutex_unlock(&p->mutex);
#endif
	pthread_mutex_lock(&p->mutex);
	int maxtime;
	pthread_mutex_unlock(&p->mutex);
	time_t ctime;
	int timeout;
	while (!p->stop && p->head){
		ctime=time(NULL);
		pthread_mutex_lock(&p->mutex);
		maxtime=onion_poller_get_next_timeout(p);
		pthread_mutex_unlock(&p->mutex);
		
		timeout=maxtime-ctime;
		if (timeout>3600)
			timeout=3600000;
		else
			timeout*=1000;
		ONION_DEBUG0("Wait for %d ms", timeout);
		int nfds = epoll_wait(p->fd, event, MAX_EVENTS, timeout);
		int ctime_end=time(NULL);
		ONION_DEBUG0("Current time is %d, limit is %d, timeout is %d. Waited for %d seconds", ctime, maxtime, timeout, ctime_end-ctime);
    ctime=ctime_end;

		pthread_mutex_lock(&p->mutex);
		{ // Somebody timedout?
			onion_poller_slot *next=p->head;
			while (next){
				onion_poller_slot *cur=next;
				next=next->next;
				if (cur->timeout_limit <= ctime){
					ONION_DEBUG0("Timeout on %d, was %d (ctime %d)", cur->fd, cur->timeout_limit, ctime);
          int i;
          for (i=0;i<nfds;i++){
            onion_poller_slot *el=(onion_poller_slot*)event[i].data.ptr;
            if (cur==el){ // If removed just one with event, make it ignore the event later.
              ONION_DEBUG0("Ignoring event as it timeouted: %d", cur->fd);
              event[i].data.ptr=NULL;
            }
          }
          onion_poller_remove(p, cur->fd);
				}
			}
		}
		pthread_mutex_unlock(&p->mutex);
		
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
			if (event[i].events&EPOLLRDHUP){
				n=-1;
			}
			else{ // I also take care of the timeout, no timeout when on the handler, it should handle it itself.
				el->timeout_limit=INT_MAX;

#ifdef __DEBUG0__
        char **bs=backtrace_symbols((void * const *)&el->f, 1);
        ONION_DEBUG0("Calling handler: %s (%d)",bs[0], el->fd);
        free(bs);
#endif
				n=el->f(el->data);
				
				ctime=time(NULL);
				if (el->timeout>0)
					el->timeout_limit=ctime+el->timeout;
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
 */
void onion_poller_stop(onion_poller *p){
  ONION_DEBUG("Stopping poller");
  p->stop=1;
  char data[8]={0,0,0,0, 0,0,0,1};
  int __attribute__((unused)) r=read(p->eventfd, data, 8); // Flush eventfd data, discard data
	
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
}

