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

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <onion/log.h>

#include <dlfcn.h>
#include "list.h"

extern list *plugin_search_path;

/**
 * @short Loads an external .so, normally at current dir, or templatetags.
 * 
 * This so has a method "void plugin_init()" that calls as necessary the
 * tag registors: tag_add(const char *tagname, void *f);
 * 
 * @returns 0 on success, -1 on error
 */
int load_external(const char *filename) {
  char tmp[256];

  list_item *it = plugin_search_path->head;
  void *dl = NULL;
  while (it) {
    snprintf(tmp, sizeof(tmp), it->data, filename);
    dl = dlopen(tmp, RTLD_NOW | RTLD_GLOBAL | RTLD_NODELETE);
    ONION_DEBUG("Check at %s %p: %s %d", tmp, dl, dlerror(), fopen(tmp, "ro"));
    if (dl)
      break;
    it = it->next;
  }
  if (!dl) {
    ONION_ERROR
        ("Could not load external library %s. Check plugin location rules at otemplate --help.",
         filename);
    return -1;
  }
  void (*f) (void) = dlsym(dl, "plugin_init");
  if (!f) {
    ONION_ERROR("Loaded library %s do not have otemplate_init", filename);
    dlclose(dl);
    return -1;
  }

  ONION_DEBUG("Executing external %s/plugin_init()", tmp);

  f();

  dlclose(dl);                  /// Closes the handle, but the library is kept open.
  return 0;
}
