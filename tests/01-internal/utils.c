/*
	Onion HTTP server library
	Copyright (C) 2016 David Moreno Montero

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

#include "utils.h"
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

#include <onion/log.h>


int connect_to(const char *addr, const char *port){
  struct addrinfo hints;
  struct addrinfo *server;

  memset(&hints,0, sizeof(struct addrinfo));
  hints.ai_socktype=SOCK_STREAM;
  hints.ai_family=AF_UNSPEC;
  hints.ai_flags=0;

  if (getaddrinfo(addr,port,&hints,&server)!=0){
    ONION_ERROR("Error getting server info");
    return -1;
  }

  struct addrinfo *next=server;
  char thost[256];
  char tport[256];
  while(next){
    int fd=socket(next->ai_family, next->ai_socktype | SOCK_CLOEXEC, next->ai_protocol);
    if (fd<0){
      ONION_ERROR("Error creating socket: %s", strerror(errno));
      return -1;
    }
    getnameinfo(next->ai_addr, next->ai_addrlen,
      thost, sizeof(thost),
      tport, sizeof(tport), NI_NUMERICHOST | NI_NUMERICSERV);
    ONION_DEBUG("Try connect at %s:%s -- %p %d", thost, tport, next, fd);
    int ok=connect(fd, next->ai_addr, next->ai_addrlen);
    if (ok==0){
      ONION_DEBUG("Done");
      freeaddrinfo(server);
      return fd;
    }
    else{
      ONION_DEBUG("Could not connect: %s", strerror(errno));
      close(fd);
    }
    next=next->ai_next;
  }

  ONION_ERROR("Error connecting to server %s:%s: %s",addr,port, strerror(errno));
  freeaddrinfo(server);
  return -1;
}
