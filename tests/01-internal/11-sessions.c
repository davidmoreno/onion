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
#include <onion/log.h>
#include <onion/sessions.h>
#include <onion/dict.h>
#include "../ctest.h"
#include "buffer_listen_point.h"
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
	onion_sessions_save(sessions, s01, ses);
	
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
	onion_listen_point *lp=onion_buffer_listen_point_new();
	onion_add_listen_point(o,NULL,NULL,lp);

  onion_request *req;
  onion_dict *session;
  char *cookieid;
  char tmp[256];
  
  req=onion_request_new(lp);
  FAIL_IF_NOT_EQUAL(req->session_id, NULL);
  session=onion_request_get_session_dict(req);
  onion_dict_add(session,"Test","tseT", 0);
  FAIL_IF_EQUAL(req->session_id, NULL);
  cookieid=strdup(req->session_id);
  onion_request_free(req);

  req=onion_request_new(lp);
  FAIL_IF_NOT_EQUAL(req->session_id, NULL);
  session=onion_request_get_session_dict(req);
  FAIL_IF_EQUAL(req->session_id, NULL);
  session=onion_request_get_session_dict(req);
  onion_dict_add(session,"Test","Another value", 0);
  FAIL_IF_EQUAL_STR(req->session_id, cookieid);
  onion_request_free(req);

  req=onion_request_new(lp);
  snprintf(tmp,sizeof(tmp),"sessionid=%s",cookieid);
  onion_dict_add(req->headers,"Cookie",tmp, OD_DUP_VALUE);
  FAIL_IF_NOT_EQUAL(req->session_id, NULL);
  session=onion_request_get_session_dict(req);
  FAIL_IF_NOT_EQUAL_STR(req->session_id, cookieid);
  FAIL_IF_NOT_EQUAL_STR(onion_dict_get(session,"Test"),"tseT");
  onion_request_free(req);

  req=onion_request_new(lp);
  snprintf(tmp,sizeof(tmp),"trashthingish=nothing interesting; sessionid=%s; wtf=ianal",cookieid);
  onion_dict_add(req->headers,"Cookie",tmp, OD_DUP_VALUE);
  FAIL_IF_NOT_EQUAL(req->session_id, NULL);
  session=onion_request_get_session_dict(req);
  FAIL_IF_NOT_EQUAL_STR(req->session_id, cookieid);
  FAIL_IF_NOT_EQUAL_STR(onion_dict_get(session,"Test"),"tseT");
  onion_request_free(req);

  req=onion_request_new(lp);
  snprintf(tmp,sizeof(tmp),"sessionid=nothing interesting; sessionid=%s; other_sessionid=ianal",cookieid);
  onion_dict_add(req->headers,"Cookie",tmp, OD_DUP_VALUE);
  FAIL_IF_NOT_EQUAL(req->session_id, NULL);
  session=onion_request_get_session_dict(req);
  FAIL_IF_NOT_EQUAL_STR(req->session_id, cookieid);
  FAIL_IF_NOT_EQUAL_STR(onion_dict_get(session,"Test"),"tseT");
  onion_request_free(req);

  req=onion_request_new(lp);
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

static char lastsessionid[256];
int has_set_cookie=0;
int set_data_on_session=0;

static onion_connection_status ask_session(void *_, onion_request *req, onion_response *res){
  onion_dict *session=onion_request_get_session_dict(req);
  if (set_data_on_session)
    onion_dict_add(session,"Test","New data to create the session",0);
  has_set_cookie=0;
  onion_response_write0(res, "If I write before getting session, then there is no Set-Cookie.\n");
  onion_response_printf(res, "%d elements at the session.\n", onion_dict_count(session));
  ONION_DEBUG("Session ID is %s, cookies %s",req->session_id, onion_request_get_header(req, "Cookie"));
  strcpy(lastsessionid, req->session_id);
  
  return OCS_PROCESSED;
}

ssize_t empty_write(onion_request *handler, const char *data, size_t length){
  ((char*)data)[length]=0;
  ONION_DEBUG("%s",data);
  if (strstr(data, "Set-Cookie:")!=NULL)
    has_set_cookie=1;
  
  return length;
}

// This fixes that whenever a session is created, but no new data is added, it does not set the proper cookie, and should not create a new session.
void t03_bug_empty_session_is_new_session(){
  INIT_LOCAL();

  onion *o=onion_new(O_ONE_LOOP);
	onion_listen_point *lp=onion_buffer_listen_point_new();
	lp->write=empty_write;
	onion_add_listen_point(o,NULL,NULL,lp);

  
  onion_url *url=onion_root_url(o);
  onion_url_add(url, "^.*", ask_session);
  char sessionid[256];
  char tmp[256];

  set_data_on_session=1;
  onion_request *req=onion_request_new(lp);
  req->fullpath="/";
  onion_request_process(req);
  FAIL_IF_EQUAL_STR(lastsessionid,"");
  strcpy(sessionid, lastsessionid);
  req->fullpath=NULL;
  onion_request_free(req);
  FAIL_IF_NOT_EQUAL_INT(onion_dict_count(o->sessions->data), 1);
  
  req=onion_request_new(lp);
  req->fullpath="/";
  onion_dict_add(req->headers, "Cookie", "sessionid=xxx", 0);
  onion_request_process(req);
  FAIL_IF_EQUAL_STR(lastsessionid,"");
  FAIL_IF_EQUAL_STR(lastsessionid, sessionid);
  FAIL_IF_NOT(has_set_cookie);
  req->fullpath=NULL;
  onion_request_free(req);
  FAIL_IF_NOT_EQUAL_INT(onion_dict_count(o->sessions->data), 2);
  
  req=onion_request_new(lp);
  req->fullpath="/";
  snprintf(tmp,sizeof(tmp),"sessionid=%s",lastsessionid);
  onion_dict_add(req->headers, "Cookie", tmp, 0);
  onion_request_process(req);
  FAIL_IF_EQUAL_STR(lastsessionid,"");
  FAIL_IF_EQUAL_STR(lastsessionid, sessionid);
  FAIL_IF_NOT(has_set_cookie);
  strcpy(sessionid, lastsessionid);
  req->fullpath=NULL;
  onion_request_free(req);
  FAIL_IF_NOT_EQUAL_INT(onion_dict_count(o->sessions->data), 2);
  
  req=onion_request_new(lp);
  req->fullpath="/";
  snprintf(tmp,sizeof(tmp),"sessionid=%sxx",lastsessionid);
  onion_dict_add(req->headers, "Cookie", tmp, 0);
  onion_request_process(req);
  FAIL_IF_EQUAL_STR(lastsessionid,"");
  FAIL_IF_EQUAL_STR(lastsessionid, sessionid);
  FAIL_IF_NOT(has_set_cookie);
  req->fullpath=NULL;
  onion_request_free(req);
  FAIL_IF_NOT_EQUAL_INT(onion_dict_count(o->sessions->data), 3);

  // Ask for new, without session data, but I will not set data on session, so session is not created.
  set_data_on_session=0;
  req=onion_request_new(lp);
  req->fullpath="/";
  onion_request_process(req);
  FAIL_IF_EQUAL_STR(lastsessionid,"");
  strcpy(sessionid, lastsessionid);
  req->fullpath=NULL;
  FAIL_IF_NOT_EQUAL_INT(onion_dict_count(o->sessions->data), 4); // For a moment it exists, until onion realizes is not necesary.
  onion_request_free(req);
  FAIL_IF_NOT_EQUAL_INT(onion_dict_count(o->sessions->data), 3);

  
  onion_free(o);
  
  END_LOCAL();
}

// This fixes that whenever a session is created, but no new data is added, it does not set the proper cookie
void t04_lot_of_sessionid(){
  INIT_LOCAL();

  onion *o=onion_new(O_ONE_LOOP);
	onion_listen_point *lp=onion_buffer_listen_point_new();
	lp->write=empty_write;
	onion_add_listen_point(o,NULL,NULL,lp);

  
  onion_url *url=onion_root_url(o);
  onion_url_add(url, "^.*", ask_session);
  char sessionid[256];
  char tmp[1024];
  char tmp2[4096];
  onion_request *req;
  int i;
  set_data_on_session=1;

  req=onion_request_new(lp);
  req->fullpath="/";
  onion_request_process(req);
  FAIL_IF_NOT_EQUAL_INT(onion_dict_count(o->sessions->data), 1);
  FAIL_IF_EQUAL_STR(lastsessionid,"");
  strcpy(sessionid, lastsessionid);
  req->fullpath=NULL;
  onion_request_free(req);

  req=onion_request_new(lp);
  req->fullpath="/";
  snprintf(tmp,sizeof(tmp)," sessionid=xx%sxx;",lastsessionid);
  strcpy(tmp2,"Cookie:");
  for(i=0;i<64;i++)
    strncat(tmp2, tmp, sizeof(tmp2)-1);
  snprintf(tmp,sizeof(tmp)," sessionid=%s\n",lastsessionid);
  strncat(tmp2, tmp, sizeof(tmp2)-1);
  ONION_DEBUG("Set cookies (%d bytes): %s",strlen(tmp2),tmp2);
  strcpy(tmp,"GET /\n");
  onion_request_write(req,tmp,strlen(tmp)); // Here is the problem, at parsing too long headers
  onion_request_write(req,tmp2,strlen(tmp2)); // Here is the problem, at parsing too long headers
	onion_request_write(req,"\n",1);
  //onion_dict_add(req->headers, "Cookie", tmp2, 0);
  
  onion_request_process(req);
  FAIL_IF_NOT_EQUAL_INT(onion_dict_count(o->sessions->data), 1);
  FAIL_IF_EQUAL_STR(lastsessionid,"");
  FAIL_IF_NOT_EQUAL_STR(lastsessionid, sessionid);
  FAIL_IF_NOT(has_set_cookie);
  onion_request_free(req);
  
  onion_free(o);
  
  END_LOCAL();
}

int main(int argc, char **argv){
  START();
  
	t01_test_session();
  t02_cookies();
  t03_bug_empty_session_is_new_session();
  t04_lot_of_sessionid();
	
	END();
}
