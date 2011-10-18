/*
	Onion HTTP server library
	Copyright (C) 2010 David Moreno Montero

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


/**
 * @mainpage libonion - HTTP server library
 * @author David Moreno Montero - Coralbits S.L. - http://www.coralbits.com
 * @warning Under AGPL 3.0 License - http://www.gnu.org/licenses/agpl.html
 * 
 * @section Introduction
 *  
 * libonion is a simple library to add HTTP functionality to your program. Be it a new program or to add HTTP
 * functionality to an exisitng program, libonion makes it as easy as possible.
 * 
 * @note Best way to navigate through this documentation is going to Files / Globals / Functions, or using the search bot on the corner.
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
  gcc -o a a.c -I$ONION_DIR/src/ -L$ONION_DIR/src/onion/ -L$ONION_DIR/src/handlers/  -lonion_handlers -lonion_static -lpam -lgnutls -lgcrypt -lpthread
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
 * to process current petition.
 * 
 * This way you can have this tree like structure:
 * 
@verbatim
 + [servername=libonion.coralbits.com] +- /login/ -- Auth -- set session -- redirect /
 |                                     +- / -- custom handler index.html, depend on session data.
 |                                     +- /favicon.ico -- return file contents.
 + static content 404 not found.
@endverbatim
 * 
 * The way to create this structure would be something like:
 * 
 * @code
 * onion_url *urls=onion_url_new();
 * 
 * onion_url_add_handler(urls, "^login$", 
 *                            onion_handler_auth_pam("libonion real,","libonion",
 *                                        onion_handler_redirect("/")
 *                            )
 *                       );
 * onion_url_add_handler(urls, "^$", custom_handler());
 * onion_url_add_handler(urls, "^favicon\\.ico$", onion_handler_static_file("/favicon.ico","favicon.ico"));
 * onion_handler *root=onion_handler_servername("libonion.coralbits.com", onion_url_to_handler(urls));
 * onion_handler_add(root, onion_handler_static("<h1>404 - not found</h1>",404));
 * @endcode
 *
 * As you can see its not as easy as some configuration files, but the point its as powerfull as you need it
 * to be.
 * 
 * Normally for simple servers its much easier, as in the upper example.
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

// To have accept4
#define _GNU_SOURCE

#include <malloc.h>
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <poll.h>
#include <arpa/inet.h>
#include <signal.h>

#ifdef HAVE_GNUTLS
#include <gcrypt.h>		/* for gcry_control */
#include <gnutls/gnutls.h>
#endif
#ifdef HAVE_PTHREADS
#include <pthread.h>
#endif

#ifdef HAVE_GNUTLS
#ifdef HAVE_PTHREADS
GCRY_THREAD_OPTION_PTHREAD_IMPL;
#endif
#endif

#include "onion.h"
#include "handler.h"
#include "server.h"
#include "types_internal.h"
#include "log.h"
#include "poller.h"
#include <netdb.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>

#ifdef HAVE_SYSTEMD
#include "sd-daemon.h"
#endif

#ifndef SOCK_CLOEXEC
#define SOCK_CLOEXEC 0
#define accept4(a,b,c,d) accept(a,b,c);
#endif

#ifdef HAVE_GNUTLS
static gnutls_session_t onion_prepare_gnutls_session(onion *o, int clientfd);
static void onion_enable_tls(onion *o);
#endif

#ifdef HAVE_PTHREADS

/// Internal data as needed by onion_request_thread
struct onion_request_thread_data_t{
	onion *o;
	int clientfd;
	const char *client_info;
	pthread_t thread_handle;
};

typedef struct onion_request_thread_data_t onion_request_thread_data;

static void *onion_request_thread(void*);
#endif

/// Internal processor of just one request.
static void onion_process_request(onion *o, int clientfd, const char *client_info);

/// Accepts a request.
static int onion_accept_request(onion *o);

/**
 * @short Creates the onion structure to fill with the server data, and later do the onion_listen()
 * @memberof onion_t
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
	
	onion *o=malloc(sizeof(onion));
	o->flags=flags&0x0FF;
	o->listenfd=0;
	o->server=onion_server_new();
	o->timeout=5000; // 5 seconds of timeout, default.
	o->port=strdup("8080");
	o->hostname=strdup("::");
	o->username=NULL;
#ifdef HAVE_GNUTLS
	o->flags|=O_SSL_AVAILABLE;
#endif
#ifdef HAVE_PTHREADS
	o->flags|=O_THREADS_AVALIABLE;
	o->max_threads=15;
	if (o->flags&O_THREADED){
		o->flags|=O_THREADS_ENABLED;
		sem_init(&o->thread_count, 0, o->max_threads);
		pthread_mutex_init(&o->mutex, NULL);
	}
#endif
	o->poller=NULL;
	
	return o;
}


/**
 * @short Removes the allocated data
 * @memberof onion_t
 */
void onion_free(onion *onion){
#ifdef HAVE_PTHREADS
	if (onion->flags&O_THREADS_ENABLED){
		int ntries=5;
		int c;
		for(;ntries--;){
			sem_getvalue(&onion->thread_count,&c);
			if (c==onion->max_threads){
				break;
			}
			ONION_INFO("Still some petitions on process (%d). Wait a little bit (%d).",c,ntries);
			sleep(1);
		}
	}
#endif
	close(onion->listenfd);

	if (onion->poller)
		onion_poller_free(onion->poller);
	
	if (onion->username)
		free(onion->username);
	
#ifdef HAVE_GNUTLS
	if (onion->flags&O_SSL_ENABLED){
		gnutls_certificate_free_credentials (onion->x509_cred);
		gnutls_dh_params_deinit(onion->dh_params);
		gnutls_priority_deinit (onion->priority_cache);
		if (!(onion->flags&O_SSL_NO_DEINIT))
			gnutls_global_deinit(); // This may cause problems if several characters use the gnutls on the same binary.
	}
#endif
	if (onion->port)
		free(onion->port);
	if (onion->hostname)
		free(onion->hostname);
	onion_server_free(onion->server);
	free(onion);
}

/**
 * @short Basic direct to socket write method.
 * @memberof onion_t
 */
int onion_write_to_socket(int *fd, const char *data, unsigned int len){
	return write((long int)fd, data, len);
}

/// Simple adaptor to call from pool threads the poller.
static void *onion_poller_adaptor(void *o){
	onion_poller_poll(((onion*)o)->poller);
	return NULL;
}

/**
 * @short Performs the listening with the given mode
 * @memberof onion_t
 *
 * This is the main loop for the onion server.
 *
 * Returns if there is any error. It returns actualy errno from the network operations. See socket for more information.
 */
int onion_listen(onion *o){
#ifdef HAVE_PTHREADS
	if (o->flags&O_DETACH_LISTEN && !(o->flags&O_DETACHED)){ // On first call it sets the variable, and then call again, this time detached.
		o->flags|=O_DETACHED;
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); // It do not need to pthread_join. No leak here.
		pthread_t listen_thread;
		pthread_create(&listen_thread, &attr,(void*(*)(void*)) onion_listen, o);
		pthread_attr_destroy(&attr);
		return 0;
	}
#endif
	
	int sockfd=0;
#ifdef HAVE_SYSTEMD
	if (o->flags&O_SYSTEMD){
		int n=sd_listen_fds(0);
		ONION_DEBUG("Checking if have systemd sockets: %d",n);
		if (n>0){ // If 0, normal startup. Else use the first LISTEN_FDS.
			ONION_DEBUG("Using systemd sockets");
			if (n>1){
				ONION_WARNING("Get more than one systemd socket descriptor. Using only the first.");
			}
			sockfd=SD_LISTEN_FDS_START+0;
		}
	}
#endif
	struct sockaddr_storage cli_addr;
	char address[64];
	if (sockfd==0){
		struct addrinfo hints;
		struct addrinfo *result, *rp;
		
		memset(&hints,0, sizeof(struct addrinfo));
		hints.ai_canonname=NULL;
		hints.ai_addr=NULL;
		hints.ai_next=NULL;
		hints.ai_socktype=SOCK_STREAM;
		hints.ai_family=AF_UNSPEC;
		hints.ai_flags=AI_PASSIVE|AI_NUMERICSERV;
		
		if (getaddrinfo(o->hostname, o->port, &hints, &result) !=0 ){
			ONION_ERROR("Error getting local address and port: %s", strerror(errno));
			return errno;
		}
		
		int optval=1;
		for(rp=result;rp!=NULL;rp=rp->ai_next){
			sockfd=socket(rp->ai_family, rp->ai_socktype | SOCK_CLOEXEC, rp->ai_protocol);
			if(SOCK_CLOEXEC == 0) { // Good compiler know how to cut this out
				int flags=fcntl(sockfd, F_GETFD);
				if (flags==-1){
					ONION_ERROR("Retrieving flags from listen socket");
				}
				flags|=FD_CLOEXEC;
				if (fcntl(sockfd, F_SETFD, flags)==-1){
					ONION_ERROR("Setting O_CLOEXEC to listen socket");
				}
			}
			if (sockfd<0) // not valid
				continue;
			if (setsockopt(sockfd,SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval) ) < 0){
				ONION_ERROR("Could not set socket options: %s",strerror(errno));
			}
			if (bind(sockfd, rp->ai_addr, rp->ai_addrlen) == 0)
				break; // Success
			close(sockfd);
		}
		if (rp==NULL){
			ONION_ERROR("Could not find any suitable address to bind to.");
			return errno;
		}

		getnameinfo(rp->ai_addr, rp->ai_addrlen, address, 32,
								&address[32], 32, NI_NUMERICHOST | NI_NUMERICSERV);
		ONION_DEBUG("Listening to %s:%s",address,&address[32]);
		freeaddrinfo(result);
		
		listen(sockfd,5); // queue of only 5.
	}
	o->listenfd=sockfd;
	// Drops priviledges as it has binded.
	if (o->username){
		struct passwd *pw;
		pw=getpwnam(o->username);
		int error;
		if (!pw){
			ONION_ERROR("Cant find user to drop priviledges: %s", o->username);
			return errno;
		}
		else{
			error=initgroups(o->username, pw->pw_gid);
			error|=setgid(pw->pw_gid);
			error|=setuid(pw->pw_uid);
		}
		if (error){
			ONION_ERROR("Cant set the uid/gid for user %s", o->username);
			return errno;
		}
	}

	socklen_t clilen = sizeof(cli_addr);

	if (o->flags&O_POLL){
		o->poller=onion_poller_new(128);
		onion_poller_add(o->poller, sockfd, (void*) onion_accept_request, o);
		// O_POLL && O_THREADED == O_POOL. Create several threads to poll.
#ifdef HAVE_PTHREADS
		if (o->flags&O_THREADED){
			ONION_WARNING("Pool mode is experimental. %d threads.", o->max_threads);
			pthread_t *thread=(pthread_t*)malloc(sizeof(pthread_t)*(o->max_threads-1));
			int i;
			for(i=0;i<o->max_threads-1;i++){
				pthread_create(&thread[i], NULL, onion_poller_adaptor, o);
			}
			onion_poller_poll(o->poller);
			for(i=0;i<o->max_threads-1;i++){
				pthread_join(thread[i], NULL);
			}
		}
		else
#endif
		{
			ONION_WARNING("Poller mode is experimental.");
			onion_poller_poll(o->poller);
		}
	}
	else if (o->flags&O_ONE){
		if (o->flags&O_ONE_LOOP){
			while(1){
				onion_accept_request(o);
			}
		}
		else{
			onion_accept_request(o);
		}
	}
#ifdef HAVE_PTHREADS
	else if (o->flags&O_THREADS_ENABLED){
		struct sockaddr_storage cli_addr;
		char address[64];
		int clientfd;
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); // It do not need to pthread_join. No leak here.
		while(1){
			clientfd=accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
			
			int flags=fcntl(clientfd, F_GETFD, 0);
			if (fcntl(clientfd, F_SETFD, flags | O_CLOEXEC) < 0){ // This is inherited by sockets returned by listen.
				ONION_ERROR("Could not set connection socket options: %s",strerror(errno));
			}

			// If more than allowed processes, it waits here blocking socket, as it should be.
			// __DEBUG__
#if 0 
			int nt;
			sem_getvalue(&o->thread_count, &nt); 
			ONION_DEBUG("%d threads working, %d max threads", o->max_threads-nt, o->max_threads);
#endif
			sem_wait(&o->thread_count); 

			getnameinfo((struct sockaddr *)&cli_addr, clilen, address, sizeof(address), 
										NULL, 0, NI_NUMERICHOST);
			
			// Has to be malloc'd. If not it wil be overwritten on next petition. The threads frees it
			onion_request_thread_data *data=malloc(sizeof(onion_request_thread_data)); 
			data->o=o;
			data->clientfd=clientfd;
			data->client_info=address;
			
			pthread_create(&data->thread_handle, &attr, onion_request_thread, data);
		}
		pthread_attr_destroy(&attr);
	}
#endif
	close(sockfd);
	return 0;
}

/**
 * @short Sets the root handler
 * @memberof onion_t
 */
void onion_set_root_handler(onion *onion, onion_handler *handler){
	onion_server_set_root_handler(onion->server, handler);
}

/**
 * @short  Sets the internal error handler
 * @memberof onion_t
 */
void onion_set_internal_error_handler(onion* server, onion_handler* handler){
	onion_server_set_internal_error_handler(server->server, handler);
}

/**
 * @short Sets the port to listen to.
 * @memberof onion_t
 * 
 * Default listen port is 8080.
 * 
 * @param server The onion server to act on.
 * @param port The number of port to listen to, or service name, as string always.
 */
void onion_set_port(onion *server, const char *port){
	if (server->port)
		free(server->port);
	server->port=strdup(port);
}

/**
 * @short Sets the hostname to listen to
 * @memberof onion_t
 * 
 * Default listen hostname is NULL, which means all.
 * 
 * @param server The onion server to act on.
 * @param hostname The numeric ip/ipv6 address or hostname. NULL if listen to all interfaces.
 */
void onion_set_hostname(onion *server, const char *hostname){
	if (server->hostname)
		free(server->hostname);
	if (hostname)
		server->hostname=strdup(hostname);
	else
		server->hostname=NULL;
}

/**
 * @short Sets the timeout, in milliseconds
 * @memberof onion_t
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
 * @memberof onion_t
 * 
 * Can only be tweaked before listen. 
 * 
 * If its modified after listen, the behaviour can be unexpected, on the sense that it may server 
 * an undetermined number of request on the range [new_max_threads, current max_threads + new_max_threads]
 */
void onion_set_max_threads(onion *onion, int max_threads){
#ifdef HAVE_PTHREADS
	onion->max_threads=max_threads;
	sem_init(&onion->thread_count, 0, max_threads);
#endif
}

static onion_request *onion_connection_start(onion *o, int clientfd, const char *client_info);
static onion_connection_status onion_connection_read(onion_request *req);
static void onion_connection_shutdown(onion_request *req);

/**
 * @short Internal accept of just one request. 
 * 
 * It might be called straight from listen, or from the epoller.
 */
static int onion_accept_request(onion *o){
	struct sockaddr_storage cli_addr;
	socklen_t clilen = sizeof(cli_addr);
	char address[64];

	int clientfd=accept4(o->listenfd, (struct sockaddr *) &cli_addr, &clilen, SOCK_CLOEXEC);
	if (clientfd<0){
		ONION_ERROR("Error accepting connection.");
		return -1;
	}
	if(SOCK_CLOEXEC == 0) { // Good compiler know how to cut this out
		int flags=fcntl(clientfd, F_GETFD);
		if (flags==-1){
			ONION_ERROR("Retrieving flags from connection");
		}
		flags|=FD_CLOEXEC;
		if (fcntl(clientfd, F_SETFD, flags)==-1){
			ONION_ERROR("Setting FD_CLOEXEC to connection");
		}
	}
	
	getnameinfo((struct sockaddr *)&cli_addr, clilen, address, sizeof(address), 
							NULL, 0, NI_NUMERICHOST);
	
	if (o->flags&O_POLL){
		onion_request *req=onion_connection_start(o, clientfd, address);
		onion_poller_add(o->poller, clientfd, (void*)onion_connection_read, req);
		onion_poller_set_shutdown(o->poller, clientfd, (void*)onion_connection_shutdown, req);
		onion_poller_set_timeout(o->poller, clientfd, o->timeout);
	}
	else{
		onion_process_request(o, clientfd, address);
	}
	return 0;
}

/// Initializes the connection, create request, sets up the SSL...
static onion_request *onion_connection_start(onion *o, int clientfd, const char *client_info){
	ONION_DEBUG("Processing request");
	// sorry all the ifdefs, but here is the worst case where i would need it.. and both are almost the same.
#ifdef HAVE_GNUTLS
	gnutls_session_t session=NULL;
	if (o->flags&O_SSL_ENABLED){
		session=onion_prepare_gnutls_session(o, clientfd);
		if (session==NULL)
			return;
	}
#endif
	signal(SIGPIPE, SIG_IGN); // FIXME. remove the thread better. Now it will try to write and fail on it.

	onion_request *req;
#ifdef HAVE_GNUTLS
	if (o->flags&O_SSL_ENABLED){
		onion_server_set_write(o->server, (onion_write)gnutls_record_send); // Im lucky, has the same signature.
		req=onion_request_new(o->server, session, client_info);
		req->socket=session;
	}
	else{
		onion_server_set_write(o->server, (onion_write)onion_write_to_socket);
		req=onion_request_new(o->server, &clientfd, client_info);
		req->socket=(void*)clientfd;
	}
#else
	req=onion_request_new(o->server, &clientfd, client_info);
	onion_server_set_write(o->server, (onion_write)onion_write_to_socket);
	req->socket=(void*)(long int)clientfd;
#endif
	req->fd=clientfd;
	if (!(o->flags&O_THREADED) || !(o->flags&O_POLL))
		onion_request_set_no_keep_alive(req);
	
	return req;
}

/// Reads a packet of data, and passes it to the request parser
static onion_connection_status onion_connection_read(onion_request *req){
	int r;
	char buffer[1500];
	errno=0;
#if HAVE_GNUTLS
	r = (o->flags&O_SSL_ENABLED)
						? gnutls_record_recv (req->socket, buffer, sizeof(buffer))
						: read((int)req->socket, buffer, sizeof(buffer));
#else
	errno=0;
	r=read((long int)req->socket, buffer, sizeof(buffer));
#endif
	if (r<=0){ // error reading.
		if (errno==ECONNRESET)
			ONION_DEBUG("Connection reset by peer."); // Ok, this is more or less normal.
		else if (errno==EAGAIN){
			return OCS_NEED_MORE_DATA;
		}
		else if (errno!=0)
			ONION_ERROR("Error reading data: %s (%d)", strerror(errno), errno);
		return OCS_INTERNAL_ERROR;
	}
	return onion_server_write_to_request(req->server, req, buffer, r);
}

/// Closes a connection
static void onion_connection_shutdown(onion_request *req){
#ifdef HAVE_GNUTLS
	if (o->flags&O_SSL_ENABLED){
		gnutls_bye (req->session, GNUTLS_SHUT_WR);
		gnutls_deinit (req->session);
		/// FIXME! missing proper shutdown of real socket.
	}
#endif
	// Make it send the FIN packet.
	shutdown(req->fd, SHUT_RDWR);
	
	if (0!=close(req->fd)){
		perror("Error closing connection");
	}
	onion_request_free(req);
}

/**
 * @short Internal processor of just one request.
 * @memberof onion_t
 *
 * It can be used on one processing, on threaded, on one_loop...
 * 
 * It reads all the input passes it to the request, until onion_request_writes returns that the 
 * connection must be closed. This function do no close the connection, but the caller must 
 * close it when it returns.
 * 
 * It also do all the SSL startup when appropiate.
 * 
 * @param o Onion struct.
 * @param clientfd is the POSIX file descriptor of the connection.
 */
static void onion_process_request(onion *o, int clientfd, const char *client_info){
	ONION_DEBUG("Processing request");
	onion_request *req=onion_connection_start(o, clientfd, client_info);
	onion_connection_status cs=OCS_KEEP_ALIVE;
	struct pollfd pfd;
	pfd.events=POLLIN;
	pfd.fd=clientfd;
	while ((cs>=0) && (poll(&pfd,1, o->timeout)>0)){
		cs=onion_connection_read(req);
	}
	onion_connection_shutdown(req);
}

/**
 * @short Returns the current flags. @see onion_mode_e
 * @memberof onion_t
 */
int onion_flags(onion *onion){
	return onion->flags;
}


#ifdef HAVE_GNUTLS
/// Initializes GNUTLS session on the given socket.
static gnutls_session_t onion_prepare_gnutls_session(onion *o, int clientfd){
	gnutls_session_t session;

  gnutls_init (&session, GNUTLS_SERVER);
  gnutls_priority_set (session, o->priority_cache);
  gnutls_credentials_set (session, GNUTLS_CRD_CERTIFICATE, o->x509_cred);
  /* request client certificate if any.
   */
  gnutls_certificate_server_set_request (session, GNUTLS_CERT_REQUEST);
  /* Set maximum compatibility mode. This is only suggested on public webservers
   * that need to trade security for compatibility
   */
  gnutls_session_enable_compatibility_mode (session);

	gnutls_transport_set_ptr (session, (gnutls_transport_ptr_t)(long) clientfd);
	int ret = gnutls_handshake (session);
	if (ret<0){ // could not handshake. assume an error.
	  ONION_ERROR("Handshake has failed (%s)", gnutls_strerror (ret));
		gnutls_deinit (session);
		return NULL;
	}
	return session;
}

/// Enables TLS on the given server.
static void onion_enable_tls(onion *o){
#ifdef HAVE_PTHREADS
	gcry_control (GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread);
#endif
	if (!(o->flags&O_USE_DEV_RANDOM)){
		gcry_control(GCRYCTL_ENABLE_QUICK_RANDOM, 0);
	}
	gnutls_global_init ();
  gnutls_certificate_allocate_credentials (&o->x509_cred);

	gnutls_dh_params_init (&o->dh_params);
  gnutls_dh_params_generate2 (o->dh_params, 1024);
  gnutls_certificate_set_dh_params (o->x509_cred, o->dh_params);
  gnutls_priority_init (&o->priority_cache, "NORMAL", NULL);
	
	o->flags|=O_SSL_ENABLED;
}
#endif


/**
 * @short Set a certificate for use in the connection
 * @memberof onion_t
 *
 * There are several certificate types available, described at onion_ssl_certificate_type_e.
 * 
 * Returns the error code. If 0, no error.
 * 
 * Most basic and normal use is something like:
 * 
 * @code
 * 
 * onion *o=onion_new(O_THREADED);
 * onion_set_certificate(o, O_SSL_CERTIFICATE_KEY, "cert.pem", "cert.key");
 * onion_set_root_handler(o, onion_handler_directory("."));
 * onion_listen(o);
 * 
 * @endcode
 * 
 * @param onion The onion structure
 * @param type Type of certificate to set
 * @param filename Filenames of certificate files
 * 
 * @see onion_ssl_certificate_type_e
 */
int onion_set_certificate(onion *onion, onion_ssl_certificate_type type, const char *filename, ...){
#ifdef HAVE_GNUTLS
	if (!(onion->flags&O_SSL_ENABLED))
		onion_enable_tls(onion);
	int r=-1;
	switch(type&0x0FF){
		case O_SSL_CERTIFICATE_CRL:
			r=gnutls_certificate_set_x509_crl_file(onion->x509_cred, filename, (type&O_SSL_DER) ? GNUTLS_X509_FMT_DER : GNUTLS_X509_FMT_PEM);
			break;
		case O_SSL_CERTIFICATE_KEY:
		{
			va_list va;
			va_start(va, filename);
			r=gnutls_certificate_set_x509_key_file(onion->x509_cred, filename, va_arg(va, const char *), 
																									(type&O_SSL_DER) ? GNUTLS_X509_FMT_DER : GNUTLS_X509_FMT_PEM);
			va_end(va);
		}
			break;
		case O_SSL_CERTIFICATE_TRUST:
			r=gnutls_certificate_set_x509_trust_file(onion->x509_cred, filename, (type&O_SSL_DER) ? GNUTLS_X509_FMT_DER : GNUTLS_X509_FMT_PEM);
			break;
		case O_SSL_CERTIFICATE_PKCS12:
		{
			va_list va;
			va_start(va, filename);
			r=gnutls_certificate_set_x509_simple_pkcs12_file(onion->x509_cred, filename,
																														(type&O_SSL_DER) ? GNUTLS_X509_FMT_DER : GNUTLS_X509_FMT_PEM,
																														va_arg(va, const char *));
			va_end(va);
		}
			break;
		default:
			ONION_ERROR("Set unknown type of certificate: %d",type);
	}
	if (r<0){
	  ONION_ERROR("Error setting the certificate (%s)", gnutls_strerror (r));
	}
	return r;
#else
	return -1; /// Support not available
#endif
}


#ifdef HAVE_PTHREADS
/// Interfaces between the pthread_create and the process request. Actually just calls it with the proper parameters.
static void *onion_request_thread(void *d){
	onion_request_thread_data *td=(onion_request_thread_data*)d;
	onion *o=td->o;
	
	//ONION_DEBUG0("Open connection %d",td->clientfd);
	onion_process_request(o,td->clientfd, td->client_info);
		
	//ONION_DEBUG0("Closed connection %d",td->clientfd);
	free(td);
	
	sem_post(&o->thread_count);
	return NULL;
}

#endif

/**
 * @short User to which drop priviledges when listening
 * @memberof onion_t
 * 
 * Drops the priviledges of current program as soon as it starts listening.
 * 
 * This is the easiest way to allow low ports and other sensitive info to be used,
 * but the proper way should be use capabilities and/or SELinux.
 */
void onion_set_user(onion *server, const char *username){
	server->username=strdup(username);
}

void onion_url_free_data(void *);

/**
 * @short If no root handler is set, creates an url handler and returns it.
 * @memberof onion_t
 * 
 * It can also check if the current root handler is a url handler, and if it is, returns it. Else returns NULL.
 */
onion_url *onion_root_url(onion *server){
	if (server->server->root_handler){
		if (server->server->root_handler->priv_data_free==onion_url_free_data) // Only available check
			return (onion_url*)server->server->root_handler;
		return NULL;
	}
	onion_url *url=onion_url_new();
	onion_set_root_handler(server, (onion_handler*)url);
	return url;
}

