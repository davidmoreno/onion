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
#define GC_THREADS 1

#include <gc.h>

#include <onion/onion.h>
#include <onion/log.h>
#include <onion/low.h>

#include <errno.h>
#include <string.h>
#include <signal.h>
#include <netdb.h>

int hello(void *p, onion_request * req, onion_response * res) {
  //onion_response_set_length(res, 11);
  onion_response_write0(res, "Hello world");
  if (onion_request_get_query(req, "1")) {
    onion_response_printf(res, "<p>Path: %s",
                          onion_request_get_query(req, "1"));
  }
  onion_response_printf(res, "<p>Client description: %s",
                        onion_request_get_client_description(req));
  return OCS_PROCESSED;
}

onion *o = NULL;

static void shutdown_server(int _) {
  if (o)
    onion_listen_stop(o);
}

void memory_allocation_error(const char *msg) {
  ONION_ERROR("memory failure: %s (%s)", msg, strerror(errno));
  exit(EXIT_FAILURE);
}

void *GC_calloc(size_t n, size_t size) {
  void *ret = onion_low_malloc(n * size);
  memset(ret, 0, n * size);
  return ret;
}

int main(int argc, char **argv) {
  onion_low_initialize_memory_allocation(GC_malloc, GC_malloc_atomic, GC_calloc,
                                         GC_realloc, GC_strdup, GC_free,
                                         memory_allocation_error);
  onion_low_initialize_threads(GC_pthread_create, GC_pthread_join,
                               GC_pthread_cancel, GC_pthread_detach,
                               GC_pthread_exit, GC_pthread_sigmask);

  signal(SIGINT, shutdown_server);
  signal(SIGTERM, shutdown_server);

  o = onion_new(O_POOL);
  onion_set_timeout(o, 5000);
  onion_set_hostname(o, "0.0.0.0");
  onion_url *urls = onion_root_url(o);

  onion_url_add_static(urls, "static", "Hello static world", HTTP_OK);
  onion_url_add(urls, "", hello);
  onion_url_add(urls, "^(.*)$", hello);

  onion_listen(o);
  onion_free(o);
  return 0;
}
