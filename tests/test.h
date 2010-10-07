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
#include <libgen.h>
#include <stdlib.h>

/**
 * @short Basic library for testing. Uses a lot of macros and global vars.
 */

int failures=0;
int successes=0;

int local_failures=0;
int local_successes=0;

#define ERROR(...) { fprintf(stderr, __VA_ARGS__); fprintf(stderr,"\n"); }

#define INIT_LOCAL() { local_failures=local_successes=0; }
#define END_LOCAL() { fprintf(stderr,"%s ends: %d succeses / %d failures\n",__FUNCTION__,local_successes,local_failures); failures+=local_failures; successes+=local_successes; }

#define END() { fprintf(stderr,"TOTAL: %d succeses / %d failures\n",successes,failures); exit(failures); }


#define FAIL_IF(v) if (v){ ERROR("%s:%d FAIL IF %s",basename(__FILE__),__LINE__,#v); local_failures++; } else { local_successes++; }
#define FAIL_IF_NOT(v) if ( !(v) ){ ERROR("%s:%d FAIL IF NOT %s",basename(__FILE__),__LINE__,#v); local_failures++; } else { local_successes++; }


#define FAIL_IF_EQUAL(A,B) if ((A)==(B)){ ERROR("%s:%d FAIL IF EQUAL %s == %s",basename(__FILE__),__LINE__,#A,#B); local_failures++; } else { local_successes++; }
#define FAIL_IF_NOT_EQUAL(A,B) if ((A)!=(B)){ ERROR("%s:%d FAIL IF NOT EQUAL %s != %s",basename(__FILE__),__LINE__,#A,#B); local_failures++; } else { local_successes++; }

#define FAIL_IF_EQUAL_STR(A,B) { const char *a=(A); const char *b=(B); if ((!a && b) || (a && !b) || strcmp(a,b)==0){ ERROR("%s:%d FAIL IF EQUAL STR %s == %s (\"%s\" == \"%s\")",basename(__FILE__),__LINE__,#A,#B,a,b); local_failures++; } else { local_successes++; } }
#define FAIL_IF_NOT_EQUAL_STR(A,B) { const char *a=(A); const char *b=(B); if ((a!=b) && ((!a || !b) || strcmp(a,b)!=0)){ ERROR("%s:%d FAIL IF NOT EQUAL STR %s != %s (\"%s\" != \"%s\")",basename(__FILE__),__LINE__,#A,#B,a,b); local_failures++; } else { local_successes++; } }
