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
#include <onion/sessions_sqlite3.h>
#include <onion/dict.h>
#include <onion/types_internal.h>
#include "../ctest.h"
#include "buffer_listen_point.h"

void t01_test_sqlite_session(){
	INIT_LOCAL();
	
	onion_sessions *sessions=onion_sessions_sqlite3_new("sessions.sqlite");
	char *sessionid=onion_sessions_create(sessions);
	
	onion_dict *data=onion_sessions_get(sessions, sessionid);
	FAIL_IF_EQUAL(data, NULL);
	onion_dict_free(data);
	
	
	data=onion_sessions_get(sessions, sessionid);
	FAIL_IF_EQUAL(data, NULL);
	FAIL_IF_NOT_EQUAL(onion_dict_get(data,"Hello"), NULL);
	
	onion_dict_add(data,"Hello","World", 0);
	onion_dict *sub=onion_dict_new();
	onion_dict_add(data,"sub",sub, OD_FREE_VALUE|OD_DICT);
	onion_dict *sub2=onion_dict_new();
	onion_dict_add(sub2,"Subdict","With content",0);
	onion_dict_add(data,"sub2",sub2, OD_FREE_VALUE|OD_DICT);
	FAIL_IF_NOT_EQUAL_STR(onion_dict_get(data,"Hello"), "World");
	
	onion_sessions_save(sessions, sessionid, data);
	onion_dict_free(data);
	onion_sessions_free(sessions);
	
	sessions=onion_sessions_sqlite3_new("sessions.sqlite");
	
	data=onion_sessions_get(sessions, sessionid);
	FAIL_IF_EQUAL(data, NULL);
	if (data){
		FAIL_IF_NOT_EQUAL_STR(onion_dict_get(data,"Hello"), "World");
	}
	onion_dict_free(data);
	onion_sessions_free(sessions);

	free(sessionid);
	
	END_LOCAL();
}

void t02_server(){
	INIT_LOCAL();
	
	char *sessionid=NULL;
	{
		onion *o=onion_new(0);
		onion_set_session_backend(o, onion_sessions_sqlite3_new("sessions.sqlite"));
		onion_sessions *s=o->sessions;
		
		sessionid=onion_sessions_create(s);
		onion_dict *session=onion_sessions_get(s, sessionid);
		onion_dict_add(session,"Test","1",0);
		onion_sessions_save(s, sessionid, session);
		onion_dict_free(session);
		onion_free(o);
	}
	
	{
		onion *o=onion_new(0);
		onion_set_session_backend(o, onion_sessions_sqlite3_new("sessions.sqlite"));
		onion_sessions *s=o->sessions;
		
		onion_dict *session=onion_sessions_get(s, sessionid);
		
		FAIL_IF_NOT_EQUAL_STR(onion_dict_get(session, "Test"), "1");
		
		onion_dict_free(session);
		onion_free(o);
	}
	free(sessionid);
	
	
	END_LOCAL();
}

int main(int argc, char **argv){
  START();
  
	t01_test_sqlite_session();
	t02_server();
	
	END();
}
