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

#include <malloc.h>
#include <errno.h>
#include <sys/epoll.h>
#include <string.h>

#include "log.h"
#include "types.h"
#include "poller.h"
#include <unistd.h>
#include <sys/time.h>

struct onion_poller_t{
	int fd;
	int stop;
	int n;

	struct onion_poller_el_t *head;
};

typedef struct onion_poller_el_t onion_poller_el;

/// Each element of the poll
/// @private
struct onion_poller_el_t{
	int fd;
	int (*f)(void*);
	void *data;

	void (*shutdown)(void*);
	void *shutdown_data;

	int delta_timeout;
	int timeout;
	
	struct onion_poller_el_t *next;
};

/**
 * @short Returns a poller object that helps polling on sockets and files
 * @memberof onion_poller_t
 *
 * This poller is implemented through epoll, but other implementations are possible 
 */
onion_poller *onion_poller_new(int n){
	onion_poller *p=malloc(sizeof(onion_poller));
	p->fd=epoll_create(n);
	if (p->fd < 0){
		ONION_ERROR("Error creating the poller. %s", strerror(errno));
		free(p);
		return NULL;
	}
	p->stop=0;
	p->head=NULL;
	p->n=0;

	return p;
}

/// @memberof onion_poller_t
void onion_poller_free(onion_poller *p){
	ONION_DEBUG0("Free onion poller");
	p->stop=1;
	close(p->fd);
	onion_poller_el *next=p->head;
	while (next){
		onion_poller_el *tnext=next->next;
		//next->shutdown(next->shutdown_data);
		free(next);
		next=tnext;
	}
	free(p);
	ONION_DEBUG0("Done");
}

/**
 * @short Adds a file descriptor to poll.
 * @memberof onion_poller_t
 *
 * When new data is available (read/write/event) the given function
 * is called with that data.
 */
int onion_poller_add(onion_poller *poller, int fd, int (*f)(void*), void *data){
	ONION_DEBUG0("Adding fd %d/nr %d for polling", fd, poller->n);

	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.events=EPOLLIN; // | EPOLLOUT | EPOLLRDHUP;
	ev.data.fd=fd;
	if (epoll_ctl(poller->fd, EPOLL_CTL_ADD, fd, &ev) < 0){
		ONION_ERROR("Error add descriptor to listen to. %s", strerror(errno));
		return 1;
	}
	onion_poller_el *nel=malloc(sizeof(onion_poller_el));
	nel->fd=fd;
	nel->f=f;
	nel->data=data;
	nel->next=NULL;
	nel->shutdown=NULL;
	nel->shutdown_data=NULL;
	nel->timeout=-1;
	nel->delta_timeout=-1;

	if (poller->head){
		onion_poller_el *next=poller->head;
		while (next->next)
			next=next->next;
		next->next=nel;
	}
	else
		poller->head=nel;

	poller->n++;

	return 0;
}

/**
 * @short Sets a shutdown for a given file descriptor
 * 
 * This shutdown will be called whenever this descriptor is removed, for example explicit call, or closed on the other side.
 * 
 * The file descriptor must be already on the poller.
 * 
 * @param poller
 * @param fd The file descriptor
 * @param f The function to call
 * @param data The data for that call
 * 
 * @returns 1 if ok, 0 if the file descriptor is not on the poller.
 */
int onion_poller_set_shutdown(onion_poller *poller, int fd, void (*f)(void*), void *data){
	onion_poller_el *next=poller->head;
	while(next){
		if (next->fd==fd){
			ONION_DEBUG0("Added shutdown callback for fd %d", fd);
			next->shutdown=f;
			next->shutdown_data=data;
			return 1;
		}
		next=next->next;
	}
	ONION_ERROR("Could not find fd %d", fd);
	return 0;
}

/**
 * @short For a given file descriptor, already in this poller, set a timeout, in ms.
 * 
 * @param poller
 * @param fd The file descriptor
 * @param timeout The timeout, in ms.
 * 
 * @return 1 if set OK, 0 if error.
 */
int onion_poller_set_timeout(onion_poller *poller, int fd, int timeout){
	onion_poller_el *next=poller->head;
	while(next){
		if (next->fd==fd){
			//ONION_DEBUG("Added timeout for fd %d; %d", fd, timeout);
			next->timeout=timeout;
			next->delta_timeout=timeout;
			return 1;
		}
		next=next->next;
	}
	ONION_ERROR("Could not find fd %d", fd);
	return 0;
}


/**
 * @short Removes a file descriptor, and all related callbacks from the listening queue
 * @memberof onion_poller_t
 */
int onion_poller_remove(onion_poller *poller, int fd){
	ONION_DEBUG0("Trying to remove fd %d/%d", fd, poller->n);
	onion_poller_el *el=poller->head;
	if (el && el->fd==fd){
		ONION_DEBUG0("Removed from head");
		
		if (epoll_ctl(poller->fd, EPOLL_CTL_DEL, el->fd, NULL) < 0){
			ONION_ERROR("Error remove descriptor to listen to. %s", strerror(errno));
			return 1;
		}
		
		poller->head=el->next;
		if (el->shutdown)
			el->shutdown(el->shutdown_data);
		free(el);
		
		poller->n--;
		return 0;
	}
	while (el->next){
		if (el->next->fd==fd){
			ONION_DEBUG0("Removed from tail");
				onion_poller_el *t=el->next;
			el->next=t->next;
			if (t->shutdown)
				t->shutdown(t->shutdown_data);
			free(t);
			poller->n--;
			return 0;
		}
		el=el->next;
	}
	
	ONION_WARNING("Trying to remove unknown fd from poller %d", fd);
	return 0;
}

int onion_poller_get_next_timeout(onion_poller *p){
	onion_poller_el *next;
	int timeout=60*60000; // Ok, minimum wakeup , once per hour.
	next=p->head;
	while(next){
		//ONION_DEBUG("Check %d %d %d", timeout, next->timeout, next->delta_timeout);
		if (next->timeout>0){
			if (next->delta_timeout>0 && next->delta_timeout<timeout)
				timeout=next->delta_timeout;
		}
		
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
 */
void onion_poller_poll(onion_poller *p){
	struct epoll_event event[MAX_EVENTS];
	//ONION_DEBUG0("Start poll of fds");

	int timeout=0;
	int elapsed=0;
	struct timeval ts, te;
	gettimeofday(&ts, NULL);
	while (!p->stop){
		timeout=onion_poller_get_next_timeout(p);
		
		int nfds = epoll_wait(p->fd, event, MAX_EVENTS, timeout);
		gettimeofday(&te, NULL);
		elapsed=((te.tv_sec-ts.tv_sec)*1000.0) + ((te.tv_usec-ts.tv_usec)/1000.0);
		//ONION_DEBUG("Real time waiting was %d ms (compared to %d of timeout). %d wakeups.", elapsed, timeout, nfds);
		ts=te;

		onion_poller_el *next=p->head;
		while (next){
			next->delta_timeout-=elapsed;
			next=next->next;
		}

		{ // Somebody timedout?
			onion_poller_el *next=p->head;
			while (next){
				onion_poller_el *cur=next;
				next=next->next;
				if (cur->timeout >= 0 && cur->delta_timeout <= 0){
					//ONION_DEBUG("Timeout on %d", cur->fd);
					onion_poller_remove(p, cur->fd);
				}
			}
		}
		
		if (nfds<0) // This is normally closed p->fd
			return;
		int i;
		for (i=0;i<nfds;i++){
			onion_poller_el *el=p->head;
			while (el && el->fd!=event[i].data.fd)
				el=el->next;
			if (!el){
				ONION_WARNING("Event on an unlistened file descriptor!");
				continue;
			}
			el->delta_timeout=el->timeout;
			// Call the callback
			//ONION_DEBUG("Calling callback for fd %d (%X %X)", el->fd, event[i].events);
			int n=el->f(el->data);
			if (n<0)
				onion_poller_remove(p, el->fd);
			//ONION_DEBUG0("--");
		}
	}
	p->stop=0;
}

/**
 * @short Marks the poller to stop ASAP
 * @memberof onion_poller_t
 */
void onion_poller_stop(onion_poller *p){
	p->stop=1;
}

