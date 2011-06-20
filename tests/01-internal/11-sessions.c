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
#include "../test.h"

void t01_test_session(){
	INIT_LOCAL();
	
	onion_sessions *sessions=onion_sessions_new();
	
	// create a session
	onion_dict *ses=onion_sessions_get(sessions, "s01");
	FAIL_IF_EQUAL(ses, NULL);

	// get another pointer to the same session
	onion_dict *ses2=onion_sessions_get(sessions, "s01");
	FAIL_IF_NOT_EQUAL(ses, ses2);
	
	//Check it is really the same
	onion_dict_add(ses, "foo", "bar", 0);
	
	FAIL_IF_NOT_EQUAL_STR("bar", onion_dict_get(ses2, "foo"));
	
	// When removed, it should refcount--
	onion_dict_free(ses2);
	ses2=onion_sessions_get(sessions, "s01");
	FAIL_IF_NOT_EQUAL(ses, ses2);

	// also all removal, should stay at the sessions
	onion_dict_free(ses);
	onion_dict_free(ses2);
	ses2=onion_sessions_get(sessions, "s01");
	FAIL_IF_NOT_EQUAL_STR("bar", onion_dict_get(ses2, "foo"));
	onion_dict_free(ses2);
	
	// Create a second session
	ses2=onion_sessions_get(sessions, "s02");
	onion_dict_add(ses2, "hello", "world", 0);
	ses=onion_sessions_get(sessions, "s01");
	
	FAIL_IF_EQUAL_STR(onion_dict_get(ses, "hello"), onion_dict_get(ses2, "hello"));
	FAIL_IF_EQUAL_STR(onion_dict_get(ses, "foo"), onion_dict_get(ses2, "foo"));
	
	onion_dict_free(ses);
	onion_dict_free(ses2);

	// And finally really remove it
	onion_sessions_remove(sessions, "s01");
	ses2=onion_sessions_get(sessions, "s01"); // this created a new one
	FAIL_IF_EQUAL_STR("bar", onion_dict_get(ses2, "foo"));
	onion_dict_free(ses2);
	
	// This should remove the sessions, and still hanging s02
	onion_sessions_free(sessions);
	
	END_LOCAL();
}


int main(int argc, char **argv){
	t01_test_session();
	
	END();
}
