/*
	Onion HTTP server library
	Copyright (C) 2010 David Moreno Montero

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

#include <stdio.h>
#include <stdarg.h>
#include <libgen.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_PTHREADS
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#endif

#include "onion_log.h"

static int onion_log_flags=0;
static pthread_mutex_t onion_log_mutex=PTHREAD_MUTEX_INITIALIZER;

enum onion_log_flags_e{
	OF_INIT=1,
	OF_NOCOLOR=2,
	OF_NODEBUG0=4,
};

/**
 * @short Logs a message to the log facility.
 * 
 * Normally to stderr. FIXME: Add more log facilities, like syslog.
 * 
 * It can be affected also by the environment variable ONION_LOG, with one or several of:
 * 
 * - "nocolor" -- then output will be without colors.
 * - "nodebug0" -- omits output of debug0
 * 
 * @param level Level of log. 
 * @param filename File where the message appeared. Usefull on debug level, but available on all.
 * @param lineno Line where the message was produced.
 * @param fmt printf-like format string and parameters.
 */
void onion_log(onion_log_level level, const char *filename, int lineno, const char *fmt, ...){
#ifdef HAVE_PTHREADS
	pthread_mutex_lock(&onion_log_mutex);
#endif
	if (!onion_log_flags){
		onion_log_flags=OF_INIT;
		const char *ol=getenv("ONION_LOG");
		if (ol){
			if (strstr("nocolor", ol))
				onion_log_flags|=OF_NOCOLOR;
			if (strstr("nodebug0", ol))
				onion_log_flags|=OF_NODEBUG0;
		}
	}
	
#ifdef __DEBUG__
	if ((level==O_DEBUG0) && (onion_log_flags&OF_NODEBUG0)){
		#ifdef HAVE_PTHREADS
			pthread_mutex_unlock(&onion_log_mutex);
		#endif
		return;
	}
#endif

	const char *levelstr[]={ "DEBUG0", "DEBUG", "INFO", "WARNING", "ERROR" };
	const char *levelcolor[]={ "\033[34m", "\033[01;34m", "\033[0m", "\033[01;33m", "\033[31m" };
	if (!(onion_log_flags&OF_NOCOLOR))
		fprintf(stderr,levelcolor[level%4]);
	
#ifdef HAVE_PTHREADS
	fprintf(stderr, "[%04X] ",(int)syscall(SYS_gettid));
#endif
	char datetime[32];
	time_t t;
	t = time(NULL);
	strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", localtime(&t));
	
	fprintf(stderr, "[%s] [%s %s:%d] ", datetime, levelstr[level%4],  
					basename((char*)filename), lineno); // I dont know why basename is char *. Please somebody tell me.
	
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	if (!(onion_log_flags&OF_NOCOLOR))
		fprintf(stderr, "\033[0m\n");
	else
		fprintf(stderr, "\n");
	#ifdef HAVE_PTHREADS
		pthread_mutex_unlock(&onion_log_mutex);
	#endif
}
