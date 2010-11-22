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

#ifndef __ONION_LOG__
#define __ONION_LOG__

#ifdef __DEBUG__
#define ONION_DEBUG(...) onion_log(O_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define ONION_DEBUG0(...) onion_log(O_DEBUG0, __FILE__, __LINE__, __VA_ARGS__)
#else
#define ONION_DEBUG(...)
#define ONION_DEBUG0(...)
#endif

#define ONION_INFO(...) onion_log(O_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define ONION_ERROR(...) onion_log(O_ERROR, __FILE__, __LINE__, __VA_ARGS__)

#ifdef __cplusplus
extern "C"{
#endif

enum onion_log_level_e{
	O_DEBUG0=0,
	O_DEBUG=1,
	O_INFO=2,
	O_ERROR=3,
};

typedef enum onion_log_level_e onion_log_level;

void onion_log(onion_log_level level, const char *filename, int lineno, const char *fmt, ...);

#ifdef __cplusplus
}
#endif


#endif
