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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include <onion/onion.h>
#include <onion/handler.h>
#include <onion/log.h>

#include <onion/handlers/exportlocal.h>
#include <onion/handlers/static.h>

onion *o = NULL;

void free_onion(int unused) {
  ONION_INFO("Closing connections");
  onion_free(o);
  exit(0);
}

int main(int argc, char **argv) {
  char *port = "8080";
  const char *dirname = ".";
  int i;
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-p") == 0) {
      port = argv[++i];
    } else
      dirname = argv[i];
  }

  onion_handler *dir = onion_handler_export_local_new(dirname);
  onion_handler_add(dir,
                    onion_handler_static("<h1>404 - File not found.</h1>",
                                         404));

  o = onion_new(O_THREADED | O_DETACH_LISTEN | O_SYSTEMD);
  onion_set_root_handler(o, dir);

  onion_set_port(o, port);

  signal(SIGINT, free_onion);
  int error = onion_listen(o);
  if (error) {
    perror("Cant create the server");
  }
  ONION_INFO("Detached. Printing a message every 120 seconds.");
  i = 0;
  while (1) {
    i++;
    ONION_INFO("Still alive. Count %d.", i);
    sleep(120);
  }

  onion_free(o);

  return 0;
}
