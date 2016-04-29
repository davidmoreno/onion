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

#include <stdio.h>
#include <stdarg.h>
#include <libgen.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>

#ifdef HAVE_PTHREADS
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#endif

#include "log.h"

/// @defgroup log Log. Functions to ease logging and debugging in onion programs

int onion_log_flags=0;

#ifdef __DEBUG__
static const char *debug0=NULL;
#endif

#ifdef HAVE_PTHREADS
static pthread_once_t is_logging_initialized = PTHREAD_ONCE_INIT;
#else
static int is_logging_initialized = 0;
#endif

void onion_log_syslog(onion_log_level level, const char *filename, int lineno, const char *fmt, ...);
void onion_log_stderr(onion_log_level level, const char *filename, int lineno, const char *fmt, ...);

static void onion_init_logging() {
	if (!onion_log_flags) {
		onion_log_flags = OF_INIT;
#ifdef __DEBUG__
		debug0 = getenv("ONION_DEBUG0");
#endif
		const char *ol = getenv("ONION_LOG");
		if (ol) {
			if (strstr(ol, "noinfo"))
				onion_log_flags |= OF_NOINFO;
			if (strstr(ol, "nodebug"))
				onion_log_flags |= OF_NODEBUG;
			if (strstr(ol, "nocolor"))
				onion_log_flags |= OF_NOCOLOR;
			if (strstr(ol, "syslog")) // Switch to syslog
				onion_log = onion_log_syslog;
		}
	}
#ifndef HAVE_PTHREADS
	is_logging_initialized = 1;
#endif
}
/**
 * @short Logs a message to the log facility.
 * @ingroup log
 *
 * Normally to stderr, but can be set to your own logger or to use syslog.
 *
 * It is actually a variable which you can set to any other log facility. By default it is to
 * onion_log_stderr, but also available is onion_log_syslog.
 *
 * @param level Level of log.
 * @param filename File where the message appeared. Usefull on debug level, but available on all.
 * @param lineno Line where the message was produced.
 * @param fmt printf-like format string and parameters.
 */
void (*onion_log)(onion_log_level level, const char *filename, int lineno, const char *fmt, ...)=onion_log_stderr;

/**
 * @short Logs to stderr.
 * @ingroup log
 *
 * It can be affected also by the environment variable ONION_LOG, with one or several of:
 *
 * - "nocolor"  -- then output will be without colors.
 * - "syslog"   -- Switchs the logging to syslog.
 * - "noinfo"   -- Do not show info lines.
 * - "nodebug"  -- When in debug mode, do not show debug lines.
 *
 * Also for DEBUG0 level, it must be explictly set with the environmental variable ONION_DEBUG0, set
 * to the names of the files to allow DEBUG0 debug info. For example:
 *
 *   export ONION_DEBUG0="onion.c url.c"
 *
 * It is thread safe.
 *
 * When compiled in release mode (no __DEBUG__ defined), DEBUG and DEBUG0 are not compiled so they do
 * not incurr in any performance penalty.
 */
void onion_log_stderr(onion_log_level level, const char *filename, int lineno, const char *fmt, ...){
#ifdef HAVE_PTHREADS
		pthread_once(&is_logging_initialized, onion_init_logging);
#else
		if(!is_logging_initialized)
			onion_init_logging();
#endif
	if ( onion_log_flags ){
		if ( ( (level==O_INFO) && ((onion_log_flags & OF_NOINFO)==OF_NOINFO) ) ||
		     ( (level==O_DEBUG) && ((onion_log_flags & OF_NODEBUG)==OF_NODEBUG) ) ){
			return;
		}
	}

  filename=basename((char*)filename);

#ifdef __DEBUG__
	if ( (level==O_DEBUG0) && (!debug0 || !strstr(debug0, filename)) ){
		return;
	}
#endif

	const char *levelstr[]={ "DEBUG0", "DEBUG", "INFO", "WARNING", "ERROR", "UNKNOWN" };
	const char *levelcolor[]={ "\033[34m", "\033[01;34m", "\033[0m", "\033[01;33m", "\033[31m", "\033[01;31m" };
	char strout[256];
	ssize_t strout_length=0;

	if (level>(sizeof(levelstr)/sizeof(levelstr[0]))-1)
		level=(sizeof(levelstr)/sizeof(levelstr[0]))-1;

#ifdef HAVE_PTHREADS
  int pid=(unsigned long long)pthread_self();
  if (!(onion_log_flags&OF_NOCOLOR))
    strout_length+=sprintf(strout+strout_length, "\033[%dm[%04X]%s ",31 + ((unsigned int)(pid))%7, pid, levelcolor[level]);
  else
    strout_length+=sprintf(strout+strout_length, "[%04X] ", pid);
#else
  if (!(onion_log_flags&OF_NOCOLOR))
    strout_length+=sprintf(strout+strout_length,"%s",levelcolor[level]);
#endif

	char datetime[32];
	time_t t;
	struct tm result;
	t = time(NULL);
	localtime_r(&t, &result);
	strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", &result);


	strout_length+=sprintf(strout+strout_length, "[%s] [%s %s:%d] ", datetime, levelstr[level],
					filename, lineno); // I dont know why basename is char *. Please somebody tell me.

	va_list ap;
	va_start(ap, fmt);
	// this one is the only one that MUST be checked for size, as before logically
	// can be no less than sizeof(strout), and his check ensures there is space for
	// following adds
	{
		size_t maxsize=sizeof(strout) - strout_length - 6;
		size_t total_size=vsnprintf(strout+strout_length, maxsize, fmt, ap);
		if (total_size>maxsize){ // Message too long, truncates it
			strout_length+=maxsize;
			strout[strout_length-1]='.';
			strout[strout_length-2]='.';
			strout[strout_length-3]='.';
		}
		else{
			strout_length+=total_size;
		}
	}

	va_end(ap);
	if (!(onion_log_flags&OF_NOCOLOR))
		strout_length+=sprintf(strout+strout_length, "\033[0m\n");
	else
		strout_length+=sprintf(strout+strout_length, "\n");

	strout[strout_length]='\0';
	// Use of write instead of fwrite, as it shoukd be atomic with kernel doing the
	// job, but fwrite can have an internal mutex. Both should do atomic
	// writes anyway, and performance tests show no diference on Linux 4.4.3
	int w=write(2, strout, strout_length);
	((void)(w)); // w unused, no prob.
}


/**
 * @short Performs the log to the syslog
 * @ingroup log
 */
void onion_log_syslog(onion_log_level level, const char *filename, int lineno, const char *fmt, ...){
	char pri[]={LOG_DEBUG, LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERR};

	if (level>=sizeof(pri))
		return;

	va_list ap;
	va_start(ap, fmt);
	vsyslog(pri[level], fmt, ap);
	va_end(ap);
}
