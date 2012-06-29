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
 */

static int failures=0;
static int successes=0;

static int local_failures=0;
static int local_successes=0;
static int ctest_log=0;

#ifndef ERROR
static const char * __attribute__((unused)) __BASENAME__="please set the START() macro at main()";
# define USE__BASENAME
# define ERROR(...) { fprintf(stderr, "%s:%d ERROR ",__BASENAME__,__LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr,"\n"); }
#endif
#ifndef INFO
# define INFO(...) { fprintf(stderr, "%s:%d INFO ",__BASENAME__,__LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr,"\n"); }
#endif

#define LOG(...) { if (ctest_log) { fprintf(stderr, "CTEST %s:%d ",__BASENAME__,__LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr,"\n"); } }

#ifdef __cplusplus
#include <exception>
#define INIT_LOCAL() { local_failures=local_successes=0; LOG("start %s",__FUNCTION__); } try{
#define END_LOCAL() } catch(const std::exception &e){ FAIL("An exeption was catched, execution of this test aborted: %s",e.what()); } \
                    catch(...){ FAIL("An exeption was catched, execution of this test aborted"); } \
                    { INFO("%s ends: %d succeses / %d failures",__FUNCTION__,local_successes,local_failures); failures+=local_failures; successes+=local_successes; }
#else
#define INIT_LOCAL() { local_failures=local_successes=0; LOG("start %s",__FUNCTION__); }
#define END_LOCAL() { INFO("%s ends: %d succeses / %d failures\n",__FUNCTION__,local_successes,local_failures); failures+=local_failures; successes+=local_successes; }
#endif

#define INIT_TEST INIT_LOCAL
#define END_TEST END_LOCAL

#ifdef USE__BASENAME
#define START() { __BASENAME__=basename((char*)__FILE__); ctest_log=getenv("CTEST_LOG")!=NULL; }
#else
#define START() {  }
#endif
#define END() { INFO("TOTAL: %d succeses / %d failures\n",successes,failures); exit(failures); }

#define FAIL(...) { ERROR(__VA_ARGS__); local_failures++; LOG("fail"); }
#define SUCCESS() { local_successes++; LOG("ok"); }

#define FAIL_IF(v) if (v){ FAIL("FAIL IF %s",#v); } else { SUCCESS(); }
#define FAIL_IF_NOT(v) if ( !(v) ){ FAIL("FAIL IF NOT %s",#v); } else { SUCCESS(); }


#define FAIL_IF_EQUAL(A,B) if ((A)==(B)){ FAIL("FAIL IF EQUAL %s == %s",#A,#B); } else { SUCCESS(); }
#define FAIL_IF_NOT_EQUAL(A,B) if ((A)!=(B)){ FAIL("FAIL IF NOT EQUAL %s != %s",#A,#B); } else { SUCCESS(); }

#define FAIL_IF_EQUAL_INT(A,B) { int a=(A), b=(B); if (a==b){ FAIL("FAIL IF EQUAL %s == %s, %d == %d",#A,#B,a,b); } else { SUCCESS(); } }
#define FAIL_IF_NOT_EQUAL_INT(A,B) { int a=(A), b=(B);  if (a!=b){ FAIL("FAIL IF NOT EQUAL %s != %s, %d != %d",#A,#B,a,b); } else { SUCCESS(); } }

#define FAIL_IF_EQUAL_STR(A,B) { const char *a=(A); const char *b=(B); if ((a == b) || ((a && b) && strcmp(a,b)==0)){ FAIL("FAIL IF EQUAL STR %s == %s (\"%s\" == \"%s\")",#A,#B,a,b); } else { SUCCESS(); } }
#define FAIL_IF_NOT_EQUAL_STR(A,B) { const char *a=(A); const char *b=(B); if ((a!=b) && ((!a || !b) || strcmp(a,b)!=0)){ FAIL("FAIL IF NOT EQUAL STR %s != %s (\"%s\" != \"%s\")",#A,#B,a,b); } else { SUCCESS(); } }

#ifdef __cplusplus
#define FAIL_IF_EQUAL_STRING(A,B) { std::string a=A; std::string b=B; if (a == b){ FAIL("FAIL IF EQUAL STRING %s == %s (\"%s\" == \"%s\")",#A,#B,a.c_str(),b.c_str()); } else { SUCCESS(); } }
#define FAIL_IF_NOT_EQUAL_STRING(A,B) { std::string a=A; std::string b=B; if (a != b){ FAIL("FAIL IF NOT EQUAL STRING %s != %s (\"%s\" != \"%s\")",#A,#B,a.c_str(),b.c_str()); } else { SUCCESS(); } }
#endif

#define FAIL_IF_STRSTR(A,B) { const char *a=(A); const char *b=(B); if (a==b || strstr(a,b)){ FAIL("FAIL IF STRSTR %s HAS %s (\"%s\" HAS \"%s\")",#A,#B,a,b); } else { SUCCESS(); } }
#define FAIL_IF_NOT_STRSTR(A,B) { const char *a=(A); const char *b=(B); if (((a==NULL) && (b==NULL)) || (!strstr(a,b))){ FAIL("FAIL IF NOT STRSTR %s HAS NOT %s (\"%s\" HAS NOT \"%s\")",#A,#B,a,b); } else { SUCCESS(); } }

#ifdef __cplusplus
#define FAIL_IF_EXCEPTION(A) { try{ A; SUCCESS(); } catch(const std::exception &e){ FAIL("FAIL IF EXCEPTION Exception raised: %s", e.what()); } catch(...){ FAIL("Exception raised"); } }
#define FAIL_IF_NOT_EXCEPTION(A) { try{ A; FAIL("FAIL IF NOT EXCEPTION Code did not throw an exception"); } catch(...){ SUCCESS(); } }
#endif

#endif //__CORALBITS_TEST_H__
