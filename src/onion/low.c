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

/* File low.c implementing low-level utilities, notably wrappers
   for memory allocation and thread creation. This should be useful
   e.g. to use libonion with the Boehm conservative garbage collector,
   see http://hboehm.info/gc/ for more. This file is contributed by
   Basile Starynkevitch (France), see http://starynkevitch.net/Basile/ for
   contact information. */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>


#ifdef HAVE_PTHREADS
#include <pthread.h>
#endif

#include "low.h"
#include "log.h"

/// @defgroup low Low level OS functions. Encapsulates some low level functions to allow other OS implementations (Boehm GC)

/* the pointers to user provided routines for memory management &
   memory failure */
static void default_memoryfailure_onion (const char *msg);
static onion_low_malloc_sigt *malloc_onion_f = malloc;
static onion_low_scalar_malloc_sigt *scalarmalloc_onion_f = malloc;
static onion_low_calloc_sigt *calloc_onion_f = calloc;
static onion_low_realloc_sigt *realloc_onion_f = realloc;
static onion_low_strdup_sigt *strdup_onion_f = strdup;
static onion_low_free_sigt *free_onion_f = free;
static onion_low_memoryfailure_sigt *memoryfailure_onion_f
  = default_memoryfailure_onion;




#ifdef HAVE_PTHREADS
/* the pointers to user provided routines for threads */
static onion_low_pthread_create_sigt *thrcreate_onion_f = pthread_create;
static onion_low_pthread_join_sigt *thrjoin_onion_f = pthread_join;
static onion_low_pthread_cancel_sigt *thrcancel_onion_f = pthread_cancel;
static onion_low_pthread_detach_sigt *thrdetach_onion_f = pthread_detach;
static onion_low_pthread_exit_sigt *threxit_onion_f = pthread_exit;
static onion_low_pthread_sigmask_sigt *thrsigmask_onion_f = pthread_sigmask;
#endif /* HAVE_PTHREADS */


/* macro in case of memory failure. */
#define MEMORY_FAILURE(Fmt,...) do {			\
  char errmsg[48];					\
  memset (errmsg, 0, sizeof(errmsg));			\
  snprintf (errmsg, sizeof(errmsg), Fmt, __VA_ARGS__);	\
  memoryfailure_onion_f (errmsg);			\
  abort (); } while(0)


/*** our default memory failure handler: ***/
static void
default_memoryfailure_onion (const char *msg)
{
  ONION_ERROR ("memory failure: %s (%s)", msg, strerror (errno));
  exit (EXIT_FAILURE);
}

/***** NEVER FAILING ALLOCATORS *****/
void *
onion_low_malloc (size_t sz)
{
  void *res = NULL;
  if (malloc_onion_f)
    res = malloc_onion_f (sz);
  else
    res = malloc (sz);
  if (!res)
    MEMORY_FAILURE ("cannot malloc %ld bytes", (long) sz);
  return res;
}

void *
onion_low_scalar_malloc (size_t sz)
{
  void *res = NULL;
  if (scalarmalloc_onion_f)
    res = scalarmalloc_onion_f (sz);
  else
    res = malloc (sz);
  if (!res)
    MEMORY_FAILURE ("cannot malloc %ld scalar bytes", (long) sz);
  return res;
}

/* Our calloc wrapper for any kind of data, even scalar ones. */
void *
onion_low_calloc (size_t nmemb, size_t size)
{
  void *res = NULL;
  if (nmemb == 0 || size == 0)
    return NULL;
  if (calloc_onion_f)
    res = calloc_onion_f (nmemb, size);
  else
    res = calloc (nmemb, size);
  if (!res)
    MEMORY_FAILURE ("cannot calloc %ld members of %ld bytes",
		    (long) nmemb, (long) size);
  return res;
}

void *
onion_low_realloc (void *ptr, size_t size)
{
  void *res = NULL;
  if (realloc_onion_f)
    res = realloc_onion_f (ptr, size);
  else
    res = realloc (ptr, size);
  if (!res)
    MEMORY_FAILURE ("cannot realloc to %ld bytes", (long) size);
  return res;
}

char *
onion_low_strdup (const char *str)
{
  char *res = NULL;
  if (!str)
    return NULL;
  if (strdup_onion_f)
    res = strdup_onion_f (str);
  else
    res = strdup (str);
  if (!res)
    MEMORY_FAILURE ("cannot strdup string of %ld bytes", (long) strlen (str));
  return res;
}

/***** POSSIBLY FAILING ALLOCATORS *****/
void *
onion_low_try_malloc (size_t sz)
{
  void *res = NULL;
  if (malloc_onion_f)
    res = malloc_onion_f (sz);
  else
    res = malloc (sz);
  return res;
}

void *
onion_low_try_scalar_malloc (size_t sz)
{
  void *res = NULL;
  if (scalarmalloc_onion_f)
    res = scalarmalloc_onion_f (sz);
  else
    res = malloc (sz);
  return res;
}

/* Our calloc wrapper for any kind of data, even scalar ones. */
void *
onion_low_try_calloc (size_t nmemb, size_t size)
{
  void *res = NULL;
  if (nmemb == 0 || size == 0)
    return NULL;
  if (calloc_onion_f)
    res = calloc_onion_f (nmemb, size);
  else
    res = calloc (nmemb, size);
  return res;
}

void *
onion_low_try_realloc (void *ptr, size_t size)
{
  void *res = NULL;
  if (realloc_onion_f)
    res = realloc_onion_f (ptr, size);
  else
    res = realloc (ptr, size);
  return res;
}

char *
onion_low_try_strdup (const char *str)
{
  char *res = NULL;
  if (!str)
    return NULL;
  if (strdup_onion_f)
    res = strdup_onion_f (str);
  else
    res = strdup (str);
  return res;
}

void
onion_low_free (void *ptr)
{
  if (!ptr)
    return;
  if (free_onion_f)
    free_onion_f (ptr);
  else
    free (ptr);
}

/* Our configurator for memory routines. To be called once before any
   other onion processing at initialization. All the routines should
   be explicitly provided. */
void onion_low_initialize_memory_allocation
  (onion_low_malloc_sigt * malloc_pf,
   onion_low_scalar_malloc_sigt * scalarmalloc_pf,
   onion_low_calloc_sigt * calloc_pf,
   onion_low_realloc_sigt * realloc_pf,
   onion_low_strdup_sigt * strdup_pf,
   onion_low_free_sigt * free_pf,
   onion_low_memoryfailure_sigt * memoryfailure_pf)
{
  malloc_onion_f = malloc_pf;
  scalarmalloc_onion_f = scalarmalloc_pf;
  calloc_onion_f = calloc_pf;
  strdup_onion_f = strdup_pf;
  realloc_onion_f = realloc_pf;
  strdup_onion_f = strdup_pf;
  free_onion_f = free_pf;
  memoryfailure_onion_f = memoryfailure_pf;
}

#ifdef HAVE_PTHREADS

int
onion_low_pthread_create (pthread_t * thread,
			 const pthread_attr_t * attr,
			 void *(*start_routine) (void *), void *arg)
{
  return thrcreate_onion_f (thread, attr, start_routine, arg);
}

int
onion_low_pthread_join (pthread_t thread, void **retval)
{
  return thrjoin_onion_f (thread, retval);
}

int
onion_low_pthread_cancel (pthread_t thread)
{
  return thrcancel_onion_f (thread);
}

int
onion_low_pthread_detach (pthread_t thread)
{
  return thrdetach_onion_f (thread);
}

void
onion_low_pthread_exit (void *retval)
{
  threxit_onion_f (retval);
}


int
onion_low_pthread_sigmask (int how, const sigset_t * set, sigset_t * oldset)
{
  return thrsigmask_onion_f (how, set, oldset);
}

void onion_low_initialize_threads
  (onion_low_pthread_create_sigt * thrcreator_pf,
   onion_low_pthread_join_sigt * thrjoiner_pf,
   onion_low_pthread_cancel_sigt * thrcanceler_pf,
   onion_low_pthread_detach_sigt * thrdetacher_pf,
   onion_low_pthread_exit_sigt * threxiter_pf,
   onion_low_pthread_sigmask_sigt * thrsigmasker_pf)
{
  thrcreate_onion_f = thrcreator_pf;
  thrjoin_onion_f = thrjoiner_pf;
  thrcancel_onion_f = thrcanceler_pf;
  thrdetach_onion_f = thrdetacher_pf;
  threxit_onion_f = threxiter_pf;
  thrsigmask_onion_f = thrsigmasker_pf;
}

#endif /* HAVE_PTHREADS */
