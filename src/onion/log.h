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

#ifndef ONION_LOG_H
#define ONION_LOG_H

#ifdef __DEBUG__
#define ONION_DEBUG(...) onion_log(O_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define ONION_DEBUG0(...) onion_log(O_DEBUG0, __FILE__, __LINE__, __VA_ARGS__)
#else
/// @short Logs a debug information. When compiled in release mode this is not compiled
/// @ingroup log
#define ONION_DEBUG(...)
/**
* @short Logs a lower debug information. When compiled in release mode this is not compiled
* @ingroup log
*
* To use it, an environmetal variable ONION_DEBUG0=filename.c is required.
* If not present it shows nothing.
*/
#define ONION_DEBUG0(...)
#endif

/// @short Logs some information
/// @ingroup log
#define ONION_INFO(...) onion_log(O_INFO, __FILE__, __LINE__, __VA_ARGS__)
/// @short Logs some warning
/// @ingroup log
#define ONION_WARNING(...) onion_log(O_WARNING, __FILE__, __LINE__, __VA_ARGS__)
/// @short Logs some error
/// @ingroup log
#define ONION_ERROR(...) onion_log(O_ERROR, __FILE__, __LINE__, __VA_ARGS__)

#ifdef __cplusplus
extern "C"{
#endif

enum onion_log_level_e{
	O_DEBUG0=0,
	O_DEBUG=1,
	O_INFO=2,
	O_WARNING=3,
	O_ERROR=4,
};

enum onion_log_flags_e{
	OF_INIT=1,
	OF_NOCOLOR=2,
	OF_SYSLOGINIT=8,
	OF_NOINFO=16,
	OF_NODEBUG=32,
};


typedef enum onion_log_level_e onion_log_level;
extern int onion_log_flags; // For some speedups, as not getting client info if not going to save it.

/// This function can be overwritten with whatever onion_log facility you want to use, same signature
extern void (*onion_log)(onion_log_level level, const char *filename, int lineno, const char *fmt, ...);

void onion_log_stderr(onion_log_level level, const char *filename, int lineno, const char *fmt, ...);
void onion_log_syslog(onion_log_level level, const char *filename, int lineno, const char *fmt, ...);

#ifdef __cplusplus
}
#endif


#endif
