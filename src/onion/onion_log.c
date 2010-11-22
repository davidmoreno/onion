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

#include "onion_log.h"

/**
 * @short Logs a message to the log facility.
 * 
 * Normally to stderr. FIXME: Add more log facilities, like syslog.
 * 
 * @param level Level of log. 
 * @param filename File where the message appeared. Usefull on debug level, but available on all.
 * @param lineno Line where the message was produced.
 * @param fmt printf-like format string and parameters.
 */
void onion_log(onion_log_level level, const char *filename, int lineno, const char *fmt, ...){
	const char *levelstr[]={ "DEBUG", "INFO", "ERROR" };
	const char *levelcolor[]={ "\033[01;34m", "\033[0m", "\033[31m" };
	char datetime[32];
	
	time_t t;
	t = time(NULL);
	strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", localtime(&t));
	
	fprintf(stderr, "%s[%s] [%s %s:%d] ",levelcolor[level%4], datetime, levelstr[level%4],  
					basename((char*)filename), lineno); // I dont know why basename is char *. Please somebody tell me.
	
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\033[0m\n");
}
