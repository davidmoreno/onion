 /*
  *  CTest -- C/C++ automatic testing in a file.
  *   Copyright (C) 2010-2012 David Moreno Montero

  *  This program is free software: you can redistribute it and/or modify
  *  it under the terms of the GNU Affero General Public License as
  *  published by the Free Software Foundation, either version 3 of the
  *  License, or (at your option) any later version.

  *  This program is distributed in the hope that it will be useful,
  *  but WITHOUT ANY WARRANTY; without even the implied warranty of
  *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  *  GNU Affero General Public License for more details.

  *  You should have received a copy of the GNU Affero General Public License
  *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
  */


#ifndef __CORALBITS_TEST_H__

#include <stdio.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>

/**
 * @short Basic library for testing. Uses a lot of macros and global vars.
 *
 * Normal usage:
 *
 * at each function start: INIT_LOCAL(), at end END_LOCAL(). At begining of main START(), at end END().
 *
 * And these are the known tests:
 *
 * - FAIL(text)
 * - FAIL_IF(cond)
 * - FAIL_IF_NOT(cond)
 * - FAIL_IF_EQUAL(A,B)
 * - FAIL_IF_NOT_EQUAL(A,B)
 * - FAIL_IF_EQUAL_INT(A,B)
 * - FAIL_IF_NOT_EQUAL_INT(A,B)
 * - FAIL_IF_EQUAL_STR(A,B)
 * - FAIL_IF_NOT_EQUAL_STR(A,B)
 * - FAIL_IF_STRSTR(A,B) -- B is contained in A.
 * - FAIL_IF_NOT_STRSTR(A,B)
 * - FAIL_IF_EXCEPTION( code )
 * - FAIL_IF_NOT_EXCEPTION( code )
 */

static int __ctest__failures=0;
static int __ctest__successes=0;

static int __ctest__local_failures=0;
static int __ctest__local_successes=0;
static int __ctest__ctest_log=0;

static char *__ctest__one_test=NULL;

#ifndef ERROR
static const char * __attribute__((unused)) __BASENAME__="please set the START() macro at main()";
# define ERROR(...) { fprintf(stderr, "%s:%d ERROR ",__BASENAME__,__LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr,"\n"); }
#endif
#ifndef INFO
# define INFO(...) { fprintf(stderr, "%s:%d INFO ",__BASENAME__,__LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr,"\n"); }
#endif

#define LOG(...) { if (__ctest__ctest_log) { fprintf(stderr, "CTEST %s:%d ",__BASENAME__,__LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr,"\n"); } }

#define CHECK_IF_TEST { \
	if (__ctest__one_test){ \
		if ( strcmp(__ctest__one_test,"--list")==0){ \
			INFO(" * %s", __FUNCTION__); \
			return; \
		} \
		if ( strstr(__FUNCTION__,__ctest__one_test)==0) \
			return; \
	} }

#ifdef __cplusplus
#include <exception>
#define INIT_LOCAL() { \
	CHECK_IF_TEST; \
	__ctest__local_failures=__ctest__local_successes=0; \
	LOG("start %s",__FUNCTION__); \
	} \
	try{
#define END_LOCAL() \
		} \
	catch(const std::exception &e){ \
		FAIL("An exeption was catched, execution of this test aborted: %s",e.what()); \
	} \
	catch(...){ \
		FAIL("An exeption was catched, execution of this test aborted"); } \
		{ INFO("%s ends: %d succeses / %d failures",__FUNCTION__,__ctest__local_successes,__ctest__local_failures); __ctest__failures+=__ctest__local_failures; __ctest__successes+=__ctest__local_successes; }
#else
#define INIT_LOCAL() { \
	CHECK_IF_TEST; \
	__ctest__local_failures=__ctest__local_successes=0; \
	INFO("Test %s",__FUNCTION__); \
	LOG("start %s",__FUNCTION__); \
	}
#define END_LOCAL() { INFO("%s ends: %d succeses / %d failures\n",__FUNCTION__,__ctest__local_successes,__ctest__local_failures); __ctest__failures+=__ctest__local_failures; __ctest__successes+=__ctest__local_successes; }
#endif

#define INIT_TEST INIT_LOCAL
#define END_TEST END_LOCAL

#define START() { \
	__BASENAME__=basename((char*)__FILE__); \
	__ctest__ctest_log=getenv("CTEST_LOG")!=NULL; \
	if (argc>1){ \
		__ctest__one_test=argv[1]; \
		if (strcmp(__ctest__one_test,"--list")==0){ \
			INFO("Listing available test functions"); \
		}	\
		else if (strcmp(__ctest__one_test,"--help")==0){ \
			INFO("%s [function to test] [--help]",argv[0]); \
			INFO("   The function to test may be specified as just a substring, and then all conforming funcitons will be tested."); \
			INFO("   It is optional, and if not given all tests are performed. Only one argument supported."); \
		} \
		else{ \
			INFO("Testing functions that match <%s> substring", __ctest__one_test); \
	} }\
}

#define END() { INFO("TOTAL: %d succeses / %d failures\n",__ctest__successes,__ctest__failures); exit(__ctest__failures); }

#define FAIL(...) { ERROR(__VA_ARGS__); __ctest__local_failures++; LOG("fail"); }
#define SUCCESS() { __ctest__local_successes++; LOG("ok"); }

#define FAIL_IF(v) if (v){ FAIL("FAIL IF %s",#v); } else { SUCCESS(); }
#define FAIL_IF_NOT(v) if ( !(v) ){ FAIL("FAIL IF NOT %s",#v); } else { SUCCESS(); }


#define FAIL_IF_EQUAL(A,B) if ((A)==(B)){ FAIL("FAIL IF EQUAL %s == %s",#A,#B); } else { SUCCESS(); }
#define FAIL_IF_NOT_EQUAL(A,B) if ((A)!=(B)){ FAIL("FAIL IF NOT EQUAL %s != %s",#A,#B); } else { SUCCESS(); }

#define FAIL_IF_EQUAL_INT(A,B) { int __a=(A), __b=(B); if (__a==__b){ FAIL("FAIL IF EQUAL %s == %s, %d == %d",#A,#B,__a,__b); } else { SUCCESS(); } }
#define FAIL_IF_NOT_EQUAL_INT(A,B) { int __a=(A), __b=(B);  if (__a!=__b){ FAIL("FAIL IF NOT EQUAL %s != %s, %d != %d",#A,#B,__a,__b); } else { SUCCESS(); } }

#define FAIL_IF_EQUAL_STR(A,B) { const char *__a=(A); const char *__b=(B); if ((__a == __b) || ((__a && __b) && strcmp(__a,__b)==0)){ FAIL("FAIL IF EQUAL STR %s == %s (\"%s\" == \"%s\")",#A,#B,__a,__b); } else { SUCCESS(); } }
#define FAIL_IF_NOT_EQUAL_STR(A,B) { const char *__a=(A); const char *__b=(B); if ((__a!=__b) && ((!__a || !__b) || strcmp(__a,__b)!=0)){ FAIL("FAIL IF NOT EQUAL STR %s != %s (\"%s\" != \"%s\")",#A,#B,__a,__b); } else { SUCCESS(); } }

#ifdef __cplusplus
#define FAIL_IF_EQUAL_STRING(A,B) { std::string __a=A; std::string __b=B; if (__a == __b){ FAIL("FAIL IF EQUAL STRING %s == %s (\"%s\" == \"%s\")",#A,#B,__a.c_str(),__b.c_str()); } else { SUCCESS(); } }
#define FAIL_IF_NOT_EQUAL_STRING(A,B) { std::string __a=A; std::string __b=B; if (__a != __b){ FAIL("FAIL IF NOT EQUAL STRING %s != %s (\"%s\" != \"%s\")",#A,#B,__a.c_str(),__b.c_str()); } else { SUCCESS(); } }
#endif

#define FAIL_IF_STRSTR(A,B) { const char *__a=(A); const char *__b=(B); if (__a==__b || strstr(__a,__b)){ FAIL("FAIL IF STRSTR %s HAS %s (\"%s\" HAS \"%s\")",#A,#B,__a,__b); } else { SUCCESS(); } }
#define FAIL_IF_NOT_STRSTR(A,B) { const char *__a=(A); const char *__b=(B); if (((__a==NULL) && (__b==NULL)) || (!strstr(__a,__b))){ FAIL("FAIL IF NOT STRSTR %s HAS NOT %s (\"%s\" HAS NOT \"%s\")",#A,#B,__a,__b); } else { SUCCESS(); } }

#ifdef __cplusplus
#define FAIL_IF_EXCEPTION(A) { try{ A; SUCCESS(); } catch(const std::exception &e){ FAIL("FAIL IF EXCEPTION Exception raised: %s", e.what()); } catch(...){ FAIL("Exception raised"); } }
#define FAIL_IF_NOT_EXCEPTION(A) { try{ A; FAIL("FAIL IF NOT EXCEPTION Code did not throw an exception"); } catch(...){ SUCCESS(); } }
#endif

#endif //__CORALBITS_TEST_H__
