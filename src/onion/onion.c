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


/**
 * @mainpage libonion - HTTP server library
 * @author David Moreno Montero - Coralbits S.L. - http://www.coralbits.com
 *
 * @section Introduction
 *
 * libonion is a simple library to add HTTP functionality to your program. Be it a new program or to add HTTP
 * functionality to an exisitng program, libonion makes it as easy as possible.
 *
 * @note Best way to navigate through this documentation is going to Files / Globals / Functions, or using the search button on the corner.
 *
 * @note To Check the C++ bindings check at the Onion namespace.
 *
 * There are many documented examples at the examples directory (https://github.com/davidmoreno/onion/tree/master/examples/).
 *
 * A basic example of a server with authentication, SSL support that serves a static directory is:
 *
 * @include examples/basic/basic.c
 *
 * To be compiled as
 *
@verbatim
  gcc -o a a.c -I$ONION_DIR/src/ -L$ONION_DIR/src/onion/ -lonion_static -lpam -lgnutls -lgcrypt -lpthread
@endverbatim
 *
 * please modify -I and -L as appropiate to your installation.
 *
 * pam, gnutls and pthread are necesary for pam auth, ssl and pthread support respectively.
 *
 * @section Handlers Onion request handlers
 *
 * libonion is based on request handlers to process all its request. The handlers may be nested creating onion
 * like layers. The first layer may be checking the server name, the second some authentication, the third
 * checks if you ask for the right path and the last returns a fixed message.
 *
 * Handlers have always a next handler that is the handler that will be called if current handler do not want
 * to process current request.
 *
 * Normally for simple servers its much easier, as in the upper example.
 *
 * @subsection Use onion_url for normal handlers
 *
 * Normal use of handler can be done using url handlers. They allow to create a
 * named url and dispatch to a handler. It allows pattern capturing at the
 * url using regex groups.
 *
 * Check documentation at the `url` module.
 *
 * @subsubsection Url_example Example of use
 *
 *
@code
onion *o=onion_new(O_POOL);
onion_url *url=onion_root_url(o);
onion_url_add(url, "/", my_index); // privdata will be NULL
onion_url_add_with_data(url, "^/profile/([^/].*)", my_profile, usersdata);
onion_url_add_static(url, "/version", "0.0.1");
onion_url_add_url(url, ...); // Nesting
@endcode
 *
 *
 * @subsection Custom_handlers Create your custom handlers
 *
 * Creating a custom handler is just to create a couple of functions with this signature:
 *
 * @code
 * int custom_handler(custom_handler_data *d, onion_request *request, onion_response *response);
 * void custom_handler_free(custom_handler_data *d);
 * @endcode
 *
 * Then create a onion_handler object with
 *
 * @code
 * onion_handler *ret=onion_handler_new((onion_handler_handler)custom_handler,
 *                                       priv_data,(onion_handler_private_data_free) custom_handler_free);
 *
 * @endcode
 *
 * Normally this creation is encapsulated in a function that also sets up the private data, and that returns
 * the onion_handler to be used where necesary.
 *
 * As you can be seen it have its own private data that is passed at creation time, and that should be freed
 * using the custom_handler_free when libonion feels its time.
 *
 * The custom_handler must then make use of the onion_response object, created from the onion_request. For example:
 *
 * @code
 * int custom_handler(custom_handler_data *d, onion_request *request, onion_response *response){
 *   onion_response_printf(response,"Hello %s","world");
 *   return OCS_PROCESSED;
 * }
 * @endcode
 *
 * @section Optional Optional library support
 *
 * libonion has support for some external library very usefull addons. Make sure you have the development
 * libraries when compiling libonion to have support for it.
 *
 * @subsection SSL SSL support
 *
 * libonion has SSL support by using GNUTLS. This is however optional, and you can disable compilation by
 * not having the development libraries, or modifing /CMakeLists.txt.
 *
 * Once you have support most basic use is just to set a certificate and key file (can be be on the same PEM file).
 * With just that you have SSL support on your server.
 *
 * @subsection Threads Thread support
 *
 * libonion has threads support. It is not heavily mutexed as this impacts performace, but basic structures that
 * are used normally are properly mutexed.
 *
 * Anyway its on your own handlers where you have to set your own mutex areas. See examples/oterm/oterm_handler.c for
 * an example on how to do it.
 *
 * Specifically its not mutexed the handler tree, as its designed to be ready on startup, and not modified in
 * all its lifespan.
 *
 * Also there is an option to listen on another thread, using the O_DETACH_LISTEN flag on onion creation.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

//#define HAVE_PTHREADS
#ifdef HAVE_PTHREADS
#include <pthread.h>
#endif

#include "low.h"
#include "types.h"
#include "onion.h"
#include "handler.h"
#include "types_internal.h"
#include "log.h"
#include "poller.h"
#include "listen_point.h"
#include "sessions.h"
#include "mime.h"
#include "http.h"
#include "https.h"

static int onion_default_error(void *handler, onion_request *req, onion_response *res);
// Import it here as I need it to know if we have a HTTP port.
ssize_t onion_http_write(onion_request *req, const char *data, size_t len);
#ifdef HAVE_GNUTLS
ssize_t onion_https_write(onion_request *req, const char *data, size_t len);
#endif

#ifndef SOCK_CLOEXEC
# define SOCK_CLOEXEC 0
#endif

/// Used by onion_sigterm handler. Only takes into account last onion.
static onion *last_onion=NULL;
static void shutdown_server(int _);

/**
 * @short Creates the onion structure to fill with the server data, and later do the onion_listen()
 * @ingroup onion
 *
 * Creates an onion structure that can be used to set the server, port, SSL and similar parameters. It works over
 * the onion structure, which is the main structure to control the listening of new connections throught TCP/IP.
 *
 * A normal usage would be like this:
 *
 * @code
 *
 * onion *o=onion_new(O_THREADED);
 * onion_set_root_handler(o, onion_handler_directory("."));
 * onion_listen(o);
 *
 * @endcode
 *
 * @param flags Or'ed flags to use at the listening daemon. Normally one of O_ONE, O_ONE_LOOP or O_THREADED.
 *
 * @returns The onion structure.
 *
 * @see onion_mode_e onion_t
 */
onion *onion_new(int flags){
	ONION_DEBUG0("Some internal sizes: onion size: %d, request size %d, response size %d",sizeof(onion),sizeof(onion_request),sizeof(onion_response));
	if(SOCK_CLOEXEC == 0){
		ONION_WARNING("There is no support for SOCK_CLOEXEC compiled in libonion. This may be a SECURITY PROBLEM as connections may leak into executed programs.");
	}

	if (!(flags&O_NO_SIGPIPE)){
		ONION_DEBUG("Ignoring SIGPIPE");
		signal(SIGPIPE, SIG_IGN);
	}


	onion *o=onion_low_calloc(1,sizeof(onion));
	if (!o){
		return NULL;
	}
	o->flags=(flags&0x0FF)|O_SSL_AVAILABLE;
	o->timeout=5000; // 5 seconds of timeout, default.
	o->poller=onion_poller_new(15);
	if (!o->poller){
		onion_low_free(o);
		return NULL;
	}
	o->sessions=onion_sessions_new();
	o->internal_error_handler=onion_handler_new((onion_handler_handler)onion_default_error, NULL, NULL);
	o->max_post_size=1024*1024; // 1MB
	o->max_file_size=1024*1024*1024; // 1GB
#ifdef HAVE_PTHREADS
	o->flags|=O_THREADS_AVAILABLE;
	o->nthreads=8;
	if (o->flags&O_THREADED)
		o->flags|=O_THREADS_ENABLED;
	pthread_mutex_init (&o->mutex, NULL);
#endif
	if (!(o->flags&O_NO_SIGTERM)){
		signal(SIGINT,shutdown_server);
		signal(SIGTERM,shutdown_server);
	}
	last_onion=o;
	return o;
}


/**
 * @short Removes the allocated data
 * @ingroup onion
 */
void onion_free(onion *onion){
	ONION_DEBUG("Onion free");

	if (onion->flags&O_LISTENING)
		onion_listen_stop(onion);

	if (onion->poller)
		onion_poller_free(onion->poller);

	if (onion->username)
		onion_low_free(onion->username);

	if (onion->listen_points){
		onion_listen_point **p=onion->listen_points;
		while(*p!=NULL){
			ONION_DEBUG("Free %p listen_point", *p);
			onion_listen_point_free(*p++);
		}
		onion_low_free(onion->listen_points);
	}
	if (onion->root_handler)
		onion_handler_free(onion->root_handler);
	if (onion->internal_error_handler)
		onion_handler_free(onion->internal_error_handler);
	onion_mime_set(NULL);
	if (onion->sessions)
		onion_sessions_free(onion->sessions);

	{
#ifdef HAVE_PTHREADS
	  pthread_mutex_lock (&onion->mutex);
#endif
	  void* data = onion->client_data;
	  onion->client_data = NULL;
	  if (data && onion->client_data_free)
	     onion->client_data_free (data);
	  onion->client_data_free = NULL;
#ifdef HAVE_PTHREADS
	  pthread_mutex_unlock (&onion->mutex);
	  pthread_mutex_destroy (&onion->mutex);
#endif
	};
#ifdef HAVE_PTHREADS
	if (onion->threads)
		onion_low_free(onion->threads);
#endif
	if (!(onion->flags&O_NO_SIGTERM)){
		signal(SIGINT,SIG_DFL);
		signal(SIGTERM,SIG_DFL);
	}
	last_onion=NULL;
	onion_low_free(onion);
}


void
onion_set_client_data (onion*server, void*data, onion_client_data_free_sig* data_free)
{
#ifdef HAVE_PTHREADS
  pthread_mutex_lock (&server->mutex);
#endif
  void *old = server->client_data;
  server->client_data = NULL;
  if (old && server->client_data_free)
    server->client_data_free(old);
  server->client_data = data;
  server->client_data_free = data_free;
#ifdef HAVE_PTHREADS
  pthread_mutex_unlock (&server->mutex);
#endif
}


void*
onion_client_data (onion*server)
{
  void* data = NULL;
  if (!server)
    return NULL;
#ifdef HAVE_PTHREADS
  pthread_mutex_lock (&server->mutex);
#endif
  data = server->client_data;
#ifdef HAVE_PTHREADS
  pthread_mutex_unlock (&server->mutex);
#endif
  return data;
}

// This marks if called more than once. but has to be cleaned after it worked,
// for using onion_listen several times in the same program
static bool shutdown_server_first_call=true;
static void shutdown_server(int _){
	if (shutdown_server_first_call){
		if (last_onion){
			onion_listen_stop(last_onion);
			ONION_INFO("Exiting onion listening (SIG%s)", _==SIGTERM ? "TERM" : "INT");
		}
	}
	else{
		ONION_ERROR("Aborting as onion does not stop listening.");
		abort();
	}
	shutdown_server_first_call=false;
}


#ifdef HAVE_PTHREADS
static pthread_mutex_t count_mtx;
static long listen_counter;
static long poller_counter;
static void* onion_listen_start (void* data)
{
  onion* o = data;
  pthread_mutex_lock (&count_mtx);
  listen_counter++;
#ifdef __DEBUG__
#ifdef __USE_GNU
  long cnt = listen_counter;
#endif /*__USE_GNU */
#endif /*__DEBUG__*/
  pthread_mutex_unlock (&count_mtx);
#ifdef __DEBUG__
#ifdef __USE_GNU
  /* GNU systems such as Linux with GNU glibc have the non-standard pthread_setname_np */
  {
    char thrname[16];
    memset (thrname, 0, sizeof(thrname));
    snprintf (thrname, sizeof(thrname), "onlisten%ld", cnt);
    pthread_setname_np (pthread_self(), thrname);
  }
#endif /*__USE_GNU */
#endif /*__DEBUG__*/
  onion_listen (o);
  return NULL;
}

long onion_count_listen_threads(void)
{
  long cnt = 0;
  pthread_mutex_lock (&count_mtx);
  cnt = listen_counter;
  pthread_mutex_unlock (&count_mtx);
  return cnt;
}

static void* onion_poller_poll_start (void* data)
{
  onion_poller* poller = data;
  pthread_mutex_lock (&count_mtx);
  poller_counter++;
#ifdef __DEBUG__
#ifdef __USE_GNU
  long cnt = poller_counter;
#endif /*__USE_GNU */
#endif /*__DEBUG__*/
  pthread_mutex_unlock (&count_mtx);
#ifdef __DEBUG__
#ifdef __USE_GNU
  /* GNU systems such as Linux with GNU glibc have the non-standard pthread_setname_np */
  {
    char thrname[16];
    memset (thrname, 0, sizeof(thrname));
    snprintf (thrname, sizeof(thrname), "onpoller%ld", cnt);
    pthread_setname_np (pthread_self(), thrname);
  }
#endif /*__USE_GNU */
#endif /*__DEBUG__*/
  onion_poller_poll(poller);
  return NULL;
}

long onion_count_poller_threads(void)
{
  long cnt = 0;
  pthread_mutex_lock (&count_mtx);
  cnt = poller_counter;
  pthread_mutex_unlock (&count_mtx);
  return cnt;
}
#endif

/**
 * @short Performs the listening with the given mode
 * @ingroup onion
 *
 * This is the main loop for the onion server.
 *
 * It initiates the listening on all the selected ports and addresses.
 *
 * @returns !=0 if there is any error. It returns actualy errno from the network operations. See socket for more information.
 */
int onion_listen(onion *o){
#ifdef HAVE_PTHREADS
	if (!(o->flags&O_DETACHED) && (o->flags&O_DETACH_LISTEN)){ // Must detach and return
		o->flags|=O_DETACHED;
		int errcode=onion_low_pthread_create(&o->listen_thread,NULL, onion_listen_start, o);
		if (errcode!=0)
			return errcode;
		return 0;
	}
#endif

	if (!o->listen_points){
		onion_add_listen_point(o,NULL,NULL,onion_http_new());
		ONION_DEBUG("Created default HTTP listen port");
	}

	/// Start listening
	size_t successful_listened_points=0;
	onion_listen_point **lp=o->listen_points;
	while (*lp){
		int listen_result=onion_listen_point_listen(*lp);
		if (!listen_result) {
			successful_listened_points++;
		}
		lp++;
	}
	if (!successful_listened_points){
		ONION_ERROR("There are no available listen points");
		return 1;
	}

	o->flags|=O_LISTENING;


	if (o->flags&O_ONE){
		onion_listen_point **listen_points=o->listen_points;
		if (listen_points[1]!=NULL){
			ONION_WARNING("Trying to use non-poll and non-thread mode with several listen points. Only the first will be listened");
		}
		onion_listen_point *op=listen_points[0];
		do{
			onion_request *req=onion_request_new(op);
			if (!req)
				continue;
			ONION_DEBUG("Accepted request %p", req);
			onion_request_set_no_keep_alive(req);
			int ret;
			do{
				ret=req->connection.listen_point->read_ready(req);
			}while(ret>=0);
			ONION_DEBUG("End of request %p", req);
			onion_request_free(req);
			//req->connection.listen_point->close(req);
		}while(((o->flags&O_ONE_LOOP) == O_ONE_LOOP) && op->listenfd>0);
	}
	else{
		onion_listen_point **listen_points=o->listen_points;
		while (*listen_points){
			onion_listen_point *p=*listen_points;
			ONION_DEBUG("Adding listen point fd %d to poller", p->listenfd);
			onion_poller_slot *slot=onion_poller_slot_new(p->listenfd, (void*)onion_listen_point_accept, p);
			onion_poller_slot_set_type(slot, O_POLL_ALL);
			onion_poller_add(o->poller, slot);
			listen_points++;
		}

#ifdef HAVE_PTHREADS
		ONION_DEBUG("Start polling / listening %p, %p, %p", o->listen_points, *o->listen_points, *(o->listen_points+1));
		if (o->flags&O_THREADED){
			o->threads=onion_low_malloc(sizeof(pthread_t)*(o->nthreads-1));
			int i;
			for (i=0;i<o->nthreads-1;i++){
				onion_low_pthread_create(&o->threads[i],NULL,onion_poller_poll_start, o->poller);
			}

			// Here is where it waits.. but eventually it will exit at onion_listen_stop
			onion_poller_poll(o->poller);
			ONION_DEBUG("Closing onion_listen");

			for (i=0;i<o->nthreads-1;i++){
				onion_low_pthread_join(o->threads[i],NULL);
			}
		}
		else
#endif
			onion_poller_poll(o->poller);

		listen_points=o->listen_points;
		while (*listen_points){
			onion_listen_point *p=*listen_points;
			if (p->listenfd>0){
				ONION_DEBUG("Removing %d from poller", p->listenfd);
				onion_poller_remove(o->poller, p->listenfd);
			}
			listen_points++;
		}
	}

	o->flags=o->flags & ~O_LISTENING;
	shutdown_server_first_call=true; // Mark as listen_stop/shutdown worked.
	return 0;
}

/**
 * @short Advises the listener to stop.
 *
 * The listener is advised to stop listening. After this call no listening is still open, and listen could be
 * called again, or the onion server freed.
 *
 * If there is any pending connection, it can finish if onion not freed before.
 */
void onion_listen_stop(onion* server){
	/// Not listening
	if ((server->flags & O_LISTENING)==0)
		return;

	/// Stop listening
	onion_listen_point **lp=server->listen_points;
	while (*lp){
		onion_listen_point_listen_stop(*lp);
		lp++;
	}
	if (server->poller){
		ONION_DEBUG("Stop listening");
		onion_poller_stop(server->poller);
	}
#ifdef HAVE_PTHREADS
	if (server->flags&O_DETACHED)
		onion_low_pthread_join(server->listen_thread, NULL);
#endif
}


/**
 * @short Sets the root handler
 * @ingroup onion
 */
void onion_set_root_handler(onion *onion, onion_handler *handler){
	onion->root_handler=handler;
}

/**
 * @short Returns current root handler.
 * @ingroup onion
 *
 * For example when changing root handler, the old one is not deleted (as oposed that when deleting the onion*
 * object it is). So user may use onion_handler_free(onion_get_root_handler(o));
 *
 * @param server The onion server
 * @returns onion_handler currently at root.
 */
onion_handler *onion_get_root_handler(onion *server){
	return server->root_handler;
}


/**
 * @short  Sets the internal error handler
 * @ingroup onion
 */
void onion_set_internal_error_handler(onion* server, onion_handler* handler){
	server->internal_error_handler=handler;
}

/**
 * @short Sets the port to listen to.
 * @ingroup onion
 *
 * Default listen point is HTTP at localhost:8080.
 *
 * @param server The onion server to act on.
 * @param port The number of port to listen to, or service name, as string always.
 */
int onion_add_listen_point(onion* server, const char* hostname, const char* port, onion_listen_point* protocol){
	if (protocol==NULL){
		ONION_ERROR("Trying to add an invalid entry point. Ignoring.");
		return -1;
	}

	protocol->server=server;
	if (hostname)
		protocol->hostname=onion_low_strdup(hostname);
	if (port)
		protocol->port=onion_low_strdup(port);

	if (server->listen_points){
		onion_listen_point **p=server->listen_points;
		int protcount=0;
		while (*p++) protcount++;
		server->listen_points=
		  (onion_listen_point**)onion_low_realloc(server->listen_points,
							 (protcount+2)*sizeof(onion_listen_point));
		server->listen_points[protcount]=protocol;
		server->listen_points[protcount+1]=NULL;
	}
	else{
		server->listen_points=onion_low_malloc(sizeof(onion_listen_point*)*2);
		server->listen_points[0]=protocol;
		server->listen_points[1]=NULL;
	}

	ONION_DEBUG("add %p listen_point (%p, %p, %p)", protocol, server->listen_points, *server->listen_points, *(server->listen_points+1));
	return 0;
}

/**
 * @short Returns the listen point n.
 *
 *
 * @param server The onion server
 * @param nlisten_point Listen point index.
 * @returns The listen point, or NULL if not that many listen points.
 */
onion_listen_point *onion_get_listen_point(onion *server, int nlisten_point){
	onion_listen_point **next=server->listen_points;
	while(next){ // I have to go one by one, as NULL is the stop marker.
		if (nlisten_point==0)
			return *next;
		nlisten_point--;
		next++;
	}
	return NULL;
}

/**
 * @short Sets the timeout, in milliseconds
 * @ingroup onion
 *
 * The default timeout is 5000 milliseconds.
 *
 * @param timeout 0 dont wait for incomming data (too strict maybe), -1 forever, clients closes connection
 */
void onion_set_timeout(onion *onion, int timeout){
	onion->timeout=timeout;
}

/**
 * @short Sets the maximum number of threads to use for requests. default 16.
 * @ingroup onion
 *
 * Can only be tweaked before listen.
 *
 * If its modified after listen, the behaviour can be unexpected, on the sense that it may server
 * an undetermined number of request on the range [new_max_threads, current max_threads + new_max_threads]
 */
void onion_set_max_threads(onion *onion, int max_threads){
#ifdef HAVE_PTHREADS
	onion->nthreads=max_threads;
	//sem_init(&onion->thread_count, 0, max_threads);
#endif
}

/**
 * @short Returns the current flags. @see onion_mode_e
 * @ingroup onion
 */
int onion_flags(onion *onion){
	return onion->flags;
}

/**
 * @short User to which drop priviledges when listening
 * @ingroup onion
 *
 * Drops the priviledges of current program as soon as it starts listening.
 *
 * This is the easiest way to allow low ports and other sensitive info to be used,
 * but the proper way should be use capabilities and/or SELinux.
 */
void onion_set_user(onion *server, const char *username){
	server->username=onion_low_strdup(username);
}

void onion_url_free_data(onion_url_data **d);

/**
 * @short If no root handler is set, creates an url handler and returns it.
 * @ingroup onion
 *
 * It can also check if the current root handler is a url handler, and if it is, returns it. Else returns NULL.
 */
onion_url *onion_root_url(onion *server){
	if (server->root_handler){
		if (server->root_handler->priv_data_free==(void*)onion_url_free_data) // Only available check
			return (onion_url*)server->root_handler;
		ONION_WARNING("Could not get root url handler, as there is another non url handler at root.");
		return NULL;
	}
	ONION_DEBUG("New root url handler");
	onion_url *url=onion_url_new();
	server->root_handler=(onion_handler*)url;
	return url;
}

/**
 * @short Returns the poller, if any
 */
onion_poller *onion_get_poller(onion *server){
	return server->poller;
}

#define ERROR_500 "<h1>500 - Internal error</h1> Check server logs or contact administrator."
#define ERROR_403 "<h1>403 - Forbidden</h1>"
#define ERROR_404 "<h1>404 - Not found</h1>"
#define ERROR_501 "<h1>501 - Not implemented</h1>"

/**
 * @short Default error printer.
 * @ingroup onion
 *
 * Ugly errors, that can be reimplemented setting a handler with onion_server_set_internal_error_handler.
 */
static int onion_default_error(void *handler, onion_request *req, onion_response *res){
	const char *msg;
	int l;
	int code;
	switch(req->flags&0x0F000){
		case OR_INTERNAL_ERROR:
			msg=ERROR_500;
			l=sizeof(ERROR_500)-1;
			code=HTTP_INTERNAL_ERROR;
			break;
		case OR_NOT_IMPLEMENTED:
			msg=ERROR_501;
			l=sizeof(ERROR_501)-1;
			code=HTTP_NOT_IMPLEMENTED;
			break;
    case OR_FORBIDDEN:
      msg=ERROR_403;
      l=sizeof(ERROR_403)-1;
      code=HTTP_FORBIDDEN;
      break;
		default:
			msg=ERROR_404;
			l=sizeof(ERROR_404)-1;
			code=HTTP_NOT_FOUND;
			break;
	}

	ONION_DEBUG0("Internally managed error: %s, code %d.", msg, code);

	onion_response_set_code(res,code);
	onion_response_set_length(res, l);
	onion_response_write_headers(res);

	onion_response_write(res,msg,l);
	return OCS_PROCESSED;
}

/// Sets the port to listen
void onion_set_port(onion *server, const char *port){
	if (server->listen_points){
		onion_low_free(server->listen_points[0]->port);
		server->listen_points[0]->port=onion_low_strdup(port);
	}
	else{
		onion_add_listen_point(server, NULL, port, onion_http_new());
	}
}

/// Sets the hostname on which to listen
void onion_set_hostname(onion *server, const char *hostname){
	if (server->listen_points){
		onion_low_free(server->listen_points[0]->hostname);
		server->listen_points[0]->hostname=onion_low_strdup(hostname);
	}
	else{
		onion_add_listen_point(server, hostname, NULL, onion_http_new());
	}
}

/// Set a certificate for use in the connection
int onion_set_certificate(onion *onion, onion_ssl_certificate_type type, const char *filename, ...){
	va_list va;
	va_start(va, filename);
	int r=onion_set_certificate_va(onion, type, filename, va);
	va_end(va);
	return r;
}

/// Set a certificate for use in the connection
int onion_set_certificate_va(onion *onion, onion_ssl_certificate_type type, const char *filename, va_list va){
#ifdef HAVE_GNUTLS
	if (!onion->listen_points){
		onion_add_listen_point(onion,NULL,NULL,onion_https_new());
	}
	else{
		onion_listen_point *first_listen_point=onion->listen_points[0];
		if (first_listen_point->write!=onion_https_write){
			if (first_listen_point->write!=onion_http_write){
				ONION_ERROR("First listen point is not HTTP not HTTPS. Refusing to promote it to HTTPS. Use proper onion_https_new.");
				return -1;
			}
			ONION_DEBUG("Promoting from HTTP to HTTPS");
			char *port=first_listen_point->port
			  ? onion_low_strdup(first_listen_point->port) : NULL;
			char *hostname=first_listen_point->hostname
			  ? onion_low_strdup(first_listen_point->hostname) : NULL;
			onion_listen_point_free(first_listen_point);
			onion_listen_point *https=onion_https_new();
			https->server = onion;
			if (NULL==https){
				ONION_ERROR("Could not promote from HTTP to HTTPS. Certificate not set.");
			}
			https->port=port;
			https->hostname=hostname;
			onion->listen_points[0]=https;
			first_listen_point=https;
		}
	}
	int r=onion_https_set_certificate_argv(onion->listen_points[0], type, filename, va);

	return r;
#else
	ONION_ERROR("GNUTLS is not enabled. Recompile onion with GNUTLS support");
	return -1;
#endif
}


/**
 * @short Set the maximum POST size on requests
 *
 * By default its 1MB of post data. This data has to be chossen carefully as this
 * data is stored in memory, and can be abused.
 *
 * @param server The onion server
 * @param max_size The maximum desired size in bytes, by default 1MB.
 */
void onion_set_max_post_size(onion *server, size_t max_size){
	server->max_post_size=max_size;
}

/**
 * @short Set the maximum FILE size on requests
 *
 * By default its 1GB of file data. This files are stored in /tmp/, and deleted when the
 * request finishes. It can fill up your hard drive if not choosen carefully.
 *
 * Internally its stored as a file_t size, so SIZE_MAX size limit applies, which may
 * depend on your architecture. (2^32-1, 2^64-1...).
 *
 * @param server The onion server
 * @param max_size The maximum desired size in bytes, by default 1GB.
 */
void onion_set_max_file_size(onion *server, size_t max_size){
	server->max_file_size=max_size;
}

/**
 * @short Sets a new sessions backend.
 *
 * By default it uses in mem sessions, but it can be set to use sqlite sessions.
 *
 * Example:
 *
 * @code
 *   onion_set_session_backend(server, onion_sessions_sqlite3_new("sessions.sqlite"));
 * @endcode
 *
 * @param server The onion server
 * @param sessions_backend The new backend
 */
void onion_set_session_backend(onion *server, onion_sessions *sessions_backend){
	onion_sessions_free(server->sessions);
	server->sessions=sessions_backend;
}
