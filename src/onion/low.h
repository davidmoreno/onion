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

#ifndef ONION_LOW_UTIL_H
#define ONION_LOW_UTIL_H

/**
 * @defgroup low Low level. Basic OS functions that can be exchanged for alternative implementations (Boehm GC.)
 *
 * File low.h provides low level utilities, notably wrapping
 * memory allocation and thread creation. Adventurous users could even
 * customize them during early initialization, e.g. when using Hans
 * Boehm conservative garbage collector from http://hboehm.info/gc/ or
 * when using their own malloc variant so we specialize general data -
 * which might contain pointers and could be allocated with GC_MALLOC
 * into scalar data which is guaranteed to be pointer free and could
 * be allocated with GC_MALLOC_ATOMIC ...
 *
 * If your server does not use an alternative implementation, feel free not
 * to use this functions, but if it might use it, please use them.
 */

#include <stdlib.h>
#include <signal.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @short NEVER FAILING MEMORY ALLOCATORS
 * @{
 *
 * These allocators should not fail: if memory is exhausted, they
 * invoke the memory failure routine then abort with a short
 * failure message.
 */
/**
 * @short Our malloc wrapper for any kind of data, including data containing pointers.
 * @ingroup low
 */
  void *onion_low_malloc(size_t sz);

/**
 * @short Our malloc wrapper for scalar data which does not contain any pointers inside.
 * @ingroup low
 *
 * Knowing that a given zone does not contain any pointer can be useful, e.g. to Hans Boehm's conservative garbage
 * collector on http://hboehm.info/gc/ using GC_MALLOC_ATOMIC....
 */
  void *onion_low_scalar_malloc(size_t sz);

/// @short Our calloc wrapper for any kind of data, even scalar one.
/// @ingroup low
  void *onion_low_calloc(size_t nmemb, size_t size);

/// @short Our realloc wrapper for any kind of data, even scalar one.
/// @ingroup low
  void *onion_low_realloc(void *ptr, size_t size);

/// @short Our strdup wrapper.
/// @ingroup low
  char *onion_low_strdup(const char *str);

/// @}

/**
 * @short POSSIBLY FAILING MEMORY ALLOCATORS
 * @{
 *
 * These allocators could fail by returning NULL. Their caller is
 * requested to handle that failure.
 */

/// @short Our malloc wrapper for any kind of data, including data containing pointers. May return NULL on fail.
/// @ingroup low
  void *onion_low_try_malloc(size_t sz);

/// @short Our malloc wrapper for scalar data which does not contain any pointers inside.  May return NULL on fail.
/// @ingroup low
  void *onion_low_try_scalar_malloc(size_t sz);

/// @short Our calloc wrapper for any kind of data, even scalar one.  May return NULL on fail.
/// @ingroup low
  void *onion_low_try_calloc(size_t nmemb, size_t size);

/// @short Our realloc wrapper for any kind of data, even scalar one. May return NULL on fail.
/// @ingroup low
  void *onion_low_try_realloc(void *ptr, size_t size);

/// @short Our strdup wrapper. May return NULL on fail.
/// @ingroup low
  char *onion_low_try_strdup(const char *str);

  /******** FREE WRAPPER ******/
/// @short Our free wrapper for any kind of data, even scalar one.
/// @ingroup low
  void onion_low_free(void *ptr);

// @}

/// @short Signatures of user configurable memory routine replacement.  @{
/// @ingroup low
  typedef void *onion_low_malloc_sigt(size_t sz);
  typedef void *onion_low_scalar_malloc_sigt(size_t sz);
  typedef void *onion_low_calloc_sigt(size_t nmemb, size_t size);
  typedef void *onion_low_realloc_sigt(void *ptr, size_t size);
  typedef char *onion_low_strdup_sigt(const char *ptr);
  typedef void onion_low_free_sigt(void *ptr);

/**
 * @short The memory failure handler is called with a short message.
 * @ingroup low
 *
 * It generally should not return, i.e. should exit, abort, or perhaps
 * setjmp....
 */
  typedef void onion_low_memoryfailure_sigt(const char *msg);
/// @}

/**
 * @short Our configurator for memory routines.
 * @ingroup low
 *
 * To be called once before any other onion processing at initialization.
 * All the routines should be explicitly provided.
 */
  void onion_low_initialize_memory_allocation
      (onion_low_malloc_sigt * mallocrout,
       onion_low_scalar_malloc_sigt * scalarmallocrout,
       onion_low_calloc_sigt * callocrout,
       onion_low_realloc_sigt * reallocrout,
       onion_low_strdup_sigt * strduprout,
       onion_low_free_sigt * freerout,
       onion_low_memoryfailure_sigt * memoryfailurerout);

/**
 * @short Configuration for pthread creation and management.
 * @ingroup low
 * @{
 * We also offer a mean to wrap thread creation, join, cancel,
 * detach, exit. This is needed for Boehm's garbage collector -
 * which has GC_pthread_create, GC_pthread_join, ... and could be
 * useful to others, e.g. for calling pthread_setname_np on Linux
 * system.  There is no need to wrap mutexes... The wrapper functions
 * can fail and their caller is expected to check for failure.
 */
#ifdef HAVE_PTHREADS
  int onion_low_pthread_create(pthread_t * thread,
                               const pthread_attr_t * attr,
                               void *(*start_routine) (void *), void *arg);
  typedef int onion_low_pthread_create_sigt(pthread_t * thread,
                                            const pthread_attr_t * attr,
                                            void *(*start_routine) (void *),
                                            void *arg);

  int onion_low_pthread_join(pthread_t thread, void **retval);
  typedef int onion_low_pthread_join_sigt(pthread_t thread, void **retval);

  int onion_low_pthread_cancel(pthread_t thread);
  typedef int onion_low_pthread_cancel_sigt(pthread_t thread);

  int onion_low_pthread_detach(pthread_t thread);
  typedef int onion_low_pthread_detach_sigt(pthread_t thread);

  void onion_low_pthread_exit(void *retval);
  typedef void onion_low_pthread_exit_sigt(void *retval);

  int onion_low_pthread_sigmask(int how, const sigset_t * set,
                                sigset_t * oldset);
  typedef int onion_low_pthread_sigmask_sigt(int how, const sigset_t * set,
                                             sigset_t * oldset);

/**
 * @short Our configurator for pthread wrapping.
 * @ingroup low
 *
 * Every routine should be provided. This initialization should happen early,
 * at the same time as onion_low_initialize_memory_allocation, and before any
 * other onion calls. If using Boehm GC you probably want to pass
 * GC_pthread_create, GC_pthread_join, etc, etc...
 */
  void onion_low_initialize_threads
      (onion_low_pthread_create_sigt * thrcreator,
       onion_low_pthread_join_sigt * thrjoiner,
       onion_low_pthread_cancel_sigt * thrcanceler,
       onion_low_pthread_detach_sigt * thrdetacher,
       onion_low_pthread_exit_sigt * threxiter,
       onion_low_pthread_sigmask_sigt * thrsigmasker);
/// @}
#endif                          /*HAVE_PTHREADS */

#ifdef __cplusplus
}                               /* end extern "C" */
#endif
#endif                          /*ONION_LOW_UTIL_H */
