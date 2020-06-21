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

#ifndef ONION_LISTEN_POINT_H
#define ONION_LISTEN_POINT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

  onion_listen_point *onion_listen_point_new();
  int onion_listen_point_listen(onion_listen_point *);
  void onion_listen_point_listen_stop(onion_listen_point * op);
  void onion_listen_point_free(onion_listen_point *);
  int onion_listen_point_accept(onion_listen_point *);
  int onion_listen_point_request_init_from_socket(onion_request * op);
  void onion_listen_point_request_close_socket(onion_request * oc);
  void onion_listen_point_set_attachment_handlers(onion_listen_point* lp,
          int (*f_auth)(onion_request*, char*),
          int (*f_open)(const char*, int, ...),
          ssize_t (*f_pread)(const char*, void*, size_t, off_t),
          ssize_t (*f_pwrite)(const char*, const void*, size_t, off_t),
          int (*f_close)(const char*),
          int (*f_unlink)(const char*));
  void onion_listen_point_set_hash_handlers(onion_listen_point* lp,
          void* (*f_new)(),
          int (*f_init)(void* ctx),
          int (*f_update)(void* ctx, const void *data, size_t len),
          int (*f_final)(unsigned char* data, void* ctx),
          void (*f_free)(void* ctx), bool multi);

  void* onion_listen_point_att_hndl_open(onion_listen_point*);
  void* onion_listen_point_att_hndl_pread(onion_listen_point*);
  void* onion_listen_point_att_hndl_close(onion_listen_point*);
  void* onion_listen_point_att_hndl_unlink(onion_listen_point*);

  void onion_listen_point_set_cache_size(onion_listen_point* lp, size_t);

  void onion_listen_point_set_context(onion_listen_point* lp, void* context);
  void* onion_listen_point_get_context(onion_listen_point* lp);

#ifdef __cplusplus
}
#endif
#endif
