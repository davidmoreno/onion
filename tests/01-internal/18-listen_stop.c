/*
	Onion HTTP server library
	Copyright (C) 2010-2011 David Moreno Montero

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Affero General Public License as
	published by the Free Software Foundation, either version 3 of the
	License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Affero General Public License for more details.

	You should have received a copy of the GNU Affero General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
	*/

#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <netdb.h>

#include <onion/onion.h>
#include <onion/log.h>

#include "../ctest.h"

onion *o;

int connect_to(const char *addr, const char *port){
  struct addrinfo hints;
  struct addrinfo *server;
  
  memset(&hints,0, sizeof(struct addrinfo));
  hints.ai_socktype=SOCK_STREAM;
  hints.ai_family=AF_UNSPEC;
  hints.ai_flags=AI_PASSIVE|AI_NUMERICSERV;

  if (getaddrinfo(addr,port,&hints,&server)<0){
    ONION_ERROR("Error getting server info");
    return -1;
  }
  int fd=socket(server->ai_family, server->ai_socktype | SOCK_CLOEXEC, server->ai_protocol);
  
  if (connect(fd, server->ai_addr, server->ai_addrlen)==-1){
    close(fd);
    fd=-1;
    ONION_ERROR("Error connecting to server %s:%s",addr,port);
  }
  
  freeaddrinfo(server);
  
  
  return fd;
}

static void shutdown_server(int _){
	if (o) 
		onion_listen_stop(o);
}

int ok_listening=0;

void *listen_thread_f(void *_){
	ok_listening=1;
	ONION_INFO("Start listening");
	onion_listen(o);
	ONION_INFO("End listening");
	ok_listening=0;
	
	return NULL;
}

void t01_stop_listening(){
	INIT_LOCAL();
	
	signal(SIGTERM, shutdown_server);
	
	o=onion_new(O_POOL);
	
	pthread_t th;
	
	pthread_create(&th, NULL, listen_thread_f, NULL);

	sleep(2);
	FAIL_IF_NOT(ok_listening);
	kill(getpid(), SIGTERM);
	sleep(2);
	FAIL_IF(ok_listening);

	pthread_join(th, NULL);
	onion_free(o);
	
	END_LOCAL();
}

void t02_stop_listening_some_petitions(){
	INIT_LOCAL();
	
	signal(SIGTERM, shutdown_server);
	
	o=onion_new(O_POOL);
	
	pthread_t th;
	
	pthread_create(&th, NULL, listen_thread_f, NULL);

	sleep(2);
	ONION_INFO("Connecting to server");
	int connfd=connect_to("localhost","8080");
	FAIL_IF( connfd < 0 );
	FAIL_IF_NOT(ok_listening);
	kill(getpid(), SIGTERM);
	sleep(2);
	FAIL_IF(ok_listening);

	pthread_join(th, NULL);
	onion_free(o);
	
	if (connfd>=0)
		close(connfd);
	END_LOCAL();
}

int main(int argc, char **argv){
	START();
	
	t01_stop_listening();
	t02_stop_listening_some_petitions();
	
	END();
}