/*
	Onion HTTP server library
	Copyright (C) 2010-2014 David Moreno Montero and othes

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

/* File low_util.c implementing low-level utilities, notably wrappers
   for memory allocation and thread creation. This should be useful
   e.g. to use libonion with the Boehm conservative garbage collector,
   see http://hboehm.info/gc/ for more. This file is contributed by
   Basile Starynkevitch see http://starynkevitch.net/Basile/ for
   more. */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef HAVE_PTHREADS
#include <pthread.h>
#endif

#include "low_util.h"
#include "log.h"

/* the pointers to user provided routines for memory management &
   memory failure */
static onionlow_malloc_sigt *malloc_userout;
static onionlow_scalar_malloc_sigt *scalarmalloc_userout;
static onionlow_calloc_sigt *calloc_userout;
static onionlow_realloc_sigt *realloc_userout;
static onionlow_strdup_sigt *strdup_userout;
static onionlow_memoryfailure_sigt *memoryfailure_userout;
static onionlow_free_sigt *free_userout;



#ifdef HAVE_PTHREADS
/* the pointers to user provided routines for threads */
static onionlow_pthread_create_sigt *thrcreate_userout;
static onionlow_pthread_join_sigt *thrjoin_userout;
static onionlow_pthread_cancel_sigt *thrcancel_userout;
static onionlow_pthread_detach_sigt *thrdetach_userout;
static onionlow_pthread_exit_sigt *threxit_userout;
static onionlow_pthread_sigmask_sigt *thrsigmask_userout;
#endif /* HAVE_PTHREADS */


/* macro in case of memory failure. */
#define MEMORY_FAILURE(Fmt,...) do {			\
  char errmsg[48];					\
  memset (errmsg, 0, sizeof(errmsg));			\
  snprintf (errmsg, sizeof(errmsg), Fmt, __VA_ARGS__);	\
  if (memoryfailure_userout)				\
    memoryfailure_userout (errmsg);			\
  ONION_ERROR("memory failure: %s", errmsg);		\
  abort (); } while(0)

/***** NEVER FAILING ALLOCATORS *****/
void *
onionlow_malloc (size_t sz)
{
  void *res = NULL;
  if (malloc_userout)
    res = malloc_userout (sz);
  else
    res = malloc (sz);
  if (!res)
    MEMORY_FAILURE ("cannot malloc %ld bytes", (long) sz);
  return res;
}

void *
onionlow_scalar_malloc (size_t sz)
{
  void *res = NULL;
  if (scalarmalloc_userout)
    res = scalarmalloc_userout (sz);
  else
    res = malloc (sz);
  if (!res)
    MEMORY_FAILURE ("cannot malloc %ld scalar bytes", (long) sz);
  return res;
}

/* Our calloc wrapper for any kind of data, even scalar ones. */
void *
onionlow_calloc (size_t nmemb, size_t size)
{
  void *res = NULL;
  if (nmemb == 0 || size == 0)
    return NULL;
  if (calloc_userout)
    res = calloc_userout (nmemb, size);
  else
    res = calloc (nmemb, size);
  if (!res)
    MEMORY_FAILURE ("cannot calloc %ld members of %ld bytes",
		    (long) nmemb, (long) size);
  return res;
}

void *
onionlow_realloc (void *ptr, size_t size)
{
  void *res = NULL;
  if (realloc_userout)
    res = realloc_userout (ptr, size);
  else
    res = realloc (ptr, size);
  if (!res)
    MEMORY_FAILURE ("cannot realloc to %ld bytes", (long) size);
  return res;
}

char *
onionlow_strdup (const char *str)
{
  char *res = NULL;
  if (!str)
    return NULL;
  if (strdup_userout)
    res = strdup_userout (str);
  else
    res = strdup (str);
  if (!res)
    MEMORY_FAILURE ("cannot strdup string of %ld bytes", (long) strlen (str));
  return res;
}

/***** POSSIBLY FAILING ALLOCATORS *****/
void *
onionlow_try_malloc (size_t sz)
{
  void *res = NULL;
  if (malloc_userout)
    res = malloc_userout (sz);
  else
    res = malloc (sz);
  return res;
}

void *
onionlow_try_scalar_malloc (size_t sz)
{
  void *res = NULL;
  if (scalarmalloc_userout)
    res = scalarmalloc_userout (sz);
  else
    res = malloc (sz);
  return res;
}

/* Our calloc wrapper for any kind of data, even scalar ones. */
void *
onionlow_try_calloc (size_t nmemb, size_t size)
{
  void *res = NULL;
  if (nmemb == 0 || size == 0)
    return NULL;
  if (calloc_userout)
    res = calloc_userout (nmemb, size);
  else
    res = calloc (nmemb, size);
  return res;
}

void *
onionlow_try_realloc (void *ptr, size_t size)
{
  void *res = NULL;
  if (realloc_userout)
    res = realloc_userout (ptr, size);
  else
    res = realloc (ptr, size);
  return res;
}

char *
onionlow_try_strdup (const char *str)
{
  char *res = NULL;
  if (!str)
    return NULL;
  if (strdup_userout)
    res = strdup_userout (str);
  else
    res = strdup (str);
  return res;
}

void
onionlow_free (void *ptr)
{
  if (!ptr)
    return;
  if (free_userout)
    free_userout (ptr);
  else
    free (ptr);
}


#ifdef HAVE_PTHREADS
/* the pointers to user provided routines for threads */
static onionlow_pthread_create_sigt *thrcreate_userout;
static onionlow_pthread_join_sigt *thrjoin_userout;
static onionlow_pthread_cancel_sigt *thrcancel_userout;
static onionlow_pthread_detach_sigt *thrdetach_userout;
static onionlow_pthread_exit_sigt *threxit_userout;
static onionlow_pthread_sigmask_sigt *thrsigmask_userout;


int
onionlow_pthread_create (pthread_t * thread,
			 const pthread_attr_t * attr,
			 void *(*start_routine) (void *), void *arg)
{
  if (thrcreate_userout)
    return thrcreate_userout (thread, attr, start_routine, arg);
  else
    return pthread_create (thread, attr, start_routine, arg);
}

int
onionlow_pthread_join (pthread_t thread, void **retval)
{
  if (thrjoin_userout)
    return thrjoin_userout (thread, retval);
  else
    return pthread_join (thread, retval);
}

int
onionlow_pthread_cancel (pthread_t thread)
{
  if (thrcancel_userout)
    return thrcancel_userout (thread);
  else
    return pthread_cancel (thread);
}

int
onionlow_pthread_detach (pthread_t thread)
{
  if (thrdetach_userout)
    return thrdetach_userout (thread);
  else
    return pthread_detach (thread);
}

void
onionlow_pthread_exit (void* retval)
{
  if (threxit_userout)
    threxit_userout (retval);
  else
    pthread_exit (retval);
}


int
onionlow_pthread_sigmask (int how, const sigset_t * set, sigset_t * oldset)
{
  if (thrsigmask_userout)
    return thrsigmask_userout (how, set, oldset);
  else
    return pthread_sigmask (how, set, oldset);
}

#endif /* HAVE_PTHREADS */
