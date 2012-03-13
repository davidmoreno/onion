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

#include <onion/onion.h>
#include <onion/sessions.h>
#include <onion/dict.h>
#include "../ctest.h"
#include <onion/types_internal.h>

void t01_test_session(){
	INIT_LOCAL();
	
	onion_sessions *sessions=onion_sessions_new();
	
	// create a session
	onion_dict *ses=onion_sessions_get(sessions, "s01");
	FAIL_IF_NOT_EQUAL(ses, NULL); // It does not auto create the sessions, to avoid problems
	char *s01=onion_sessions_create(sessions);
	ses=onion_sessions_get(sessions, s01);
	FAIL_IF_EQUAL(ses, NULL); // It does not auto create the sessions, to avoid problems

	// get another pointer to the same session
	onion_dict *ses2=onion_sessions_get(sessions, s01);
	FAIL_IF_NOT_EQUAL(ses, ses2);
	
	//Check it is really the same
	onion_dict_add(ses, "foo", "bar", 0);
	
	FAIL_IF_NOT_EQUAL_STR("bar", onion_dict_get(ses2, "foo"));
	
	// When removed, it should refcount--
	onion_dict_free(ses2);
	ses2=onion_sessions_get(sessions, s01);
	FAIL_IF_NOT_EQUAL(ses, ses2);

	// also all removal, should stay at the sessions
	onion_dict_free(ses);
	onion_dict_free(ses2);
	ses2=onion_sessions_get(sessions, s01);
	FAIL_IF_NOT_EQUAL_STR("bar", onion_dict_get(ses2, "foo"));
	onion_dict_free(ses2);
	
	// Create a second session
	char *s02=onion_sessions_create(sessions);
	ses2=onion_sessions_get(sessions, s02);
	onion_dict_add(ses2, "hello", "world", 0);
	ses=onion_sessions_get(sessions, s01);
	
	FAIL_IF_EQUAL_STR(onion_dict_get(ses, "hello"), onion_dict_get(ses2, "hello"));
	FAIL_IF_EQUAL_STR(onion_dict_get(ses, "foo"), onion_dict_get(ses2, "foo"));
	
	onion_dict_free(ses);
	onion_dict_free(ses2);

	// And finally really remove it
	onion_sessions_remove(sessions, s01);
	ses2=onion_sessions_get(sessions, s01); 
	FAIL_IF_NOT_EQUAL(ses2, NULL);
	
	// this created a new one
	free(s01);
	s01=onion_sessions_create(sessions);
	ses2=onion_sessions_get(sessions, s01); 
	FAIL_IF_EQUAL_STR("bar", onion_dict_get(ses2, "foo"));
	onion_dict_free(ses2);
	
	// This should remove the sessions, and still hanging s02
	onion_sessions_free(sessions);
	
	free(s01);
	free(s02);
	
	END_LOCAL();
}


void t02_cookies(){
  INIT_LOCAL();
  
  onion *o=onion_new(O_ONE_LOOP);
  onion_request *req;
  onion_dict *session;
  char *cookieid;
  char tmp[256];
  
  req=onion_request_new(o->server, NULL, NULL);
  FAIL_IF_NOT_EQUAL(req->session_id, NULL);
  session=onion_request_get_session_dict(req);
  onion_dict_add(session,"Test","tseT", 0);
  FAIL_IF_EQUAL(req->session_id, NULL);
  cookieid=strdup(req->session_id);
  onion_request_free(req);

  req=onion_request_new(o->server, NULL, NULL);
  FAIL_IF_NOT_EQUAL(req->session_id, NULL);
  session=onion_request_get_session_dict(req);
  FAIL_IF_EQUAL(req->session_id, NULL);
  session=onion_request_get_session_dict(req);
  onion_dict_add(session,"Test","Another value", 0);
  FAIL_IF_EQUAL_STR(req->session_id, cookieid);
  onion_request_free(req);

  req=onion_request_new(o->server, NULL, NULL);
  snprintf(tmp,sizeof(tmp),"sessionid=%s",cookieid);
  onion_dict_add(req->headers,"Cookie",tmp, OD_DUP_VALUE);
  FAIL_IF_NOT_EQUAL(req->session_id, NULL);
  session=onion_request_get_session_dict(req);
  FAIL_IF_NOT_EQUAL_STR(req->session_id, cookieid);
  FAIL_IF_NOT_EQUAL_STR(onion_dict_get(session,"Test"),"tseT");
  onion_request_free(req);

  req=onion_request_new(o->server, NULL, NULL);
  snprintf(tmp,sizeof(tmp),"trashthingish=nothing interesting; sessionid=%s; wtf=ianal",cookieid);
  onion_dict_add(req->headers,"Cookie",tmp, OD_DUP_VALUE);
  FAIL_IF_NOT_EQUAL(req->session_id, NULL);
  session=onion_request_get_session_dict(req);
  FAIL_IF_NOT_EQUAL_STR(req->session_id, cookieid);
  FAIL_IF_NOT_EQUAL_STR(onion_dict_get(session,"Test"),"tseT");
  onion_request_free(req);

  req=onion_request_new(o->server, NULL, NULL);
  snprintf(tmp,sizeof(tmp),"sessionid=nothing interesting; sessionid=%s; other_sessionid=ianal",cookieid);
  onion_dict_add(req->headers,"Cookie",tmp, OD_DUP_VALUE);
  FAIL_IF_NOT_EQUAL(req->session_id, NULL);
  session=onion_request_get_session_dict(req);
  FAIL_IF_NOT_EQUAL_STR(req->session_id, cookieid);
  FAIL_IF_NOT_EQUAL_STR(onion_dict_get(session,"Test"),"tseT");
  onion_request_free(req);

  req=onion_request_new(o->server, NULL, NULL);
  snprintf(tmp,sizeof(tmp),"sessionid=nothing interesting; xsessionid=%s; other_sessionid=ianal",cookieid);
  onion_dict_add(req->headers,"Cookie",tmp, OD_DUP_VALUE);
  FAIL_IF_NOT_EQUAL(req->session_id, NULL);
  session=onion_request_get_session_dict(req);
  FAIL_IF_EQUAL_STR(req->session_id, cookieid);
  FAIL_IF_EQUAL_STR(onion_dict_get(session,"Test"),"tseT");
  onion_request_free(req);

  onion_free(o);
  free(cookieid);
  
  END_LOCAL();
}

int main(int argc, char **argv){
  START();
  
	t01_test_session();
  t02_cookies();
	
	END();
}
