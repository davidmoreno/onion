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

#include <stdlib.h>

#include <onion/codecs.h>

#include "../tags.h"
#include "../functions.h"

/// Following text is for gettext
void tag_trans(parser_status * st, list * l) {
  char *s = onion_c_quote_new(tag_value_arg(l, 1));
  function_add_code(st,
                    "  onion_response_write0(res, dgettext(onion_dict_get(context, \"LANG\"), %s));\n",
                    s);
  free(s);
}

/// Adds the tags.
void plugin_init() {
  tag_add("trans", tag_trans);
}
