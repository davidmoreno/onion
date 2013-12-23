/*
	Onion HTTP server library
	Copyright (C) 2010-2013 David Moreno Montero

	This library is free software; you can redistribute it and/or
	modify it under the terms of, at your choice:
	
	a. the GNU Lesser General Public License as published by the 
	 Free Software Foundation; either version 3.0 of the License, 
	 or (at your option) any later version.
	
	b. the GNU General Public License as published by the 
	 Free Software Foundation; either version 2.0 of the License, 
	 or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License and the GNU General Public License along with this 
	library; if not see <http://www.gnu.org/licenses/>.
	*/

#include <stdlib.h>

#include "sessions.h"
#include "types_internal.h"
#include "dict.h"
#include "log.h"
#include "random.h"

/**
 * @short Generates a unique id.
 * @memberof onion_sessions_t
 * 
 * This unique id is also dificult to guess, so that blind guessing will not work.
 * 
 * Just now random 32 bytes string with alphanum chars. Not really safe as using simple C rand.
 * 
 * The memory is malloc'ed and will be freed somewhere.
 */
char *onion_sessions_generate_id(){
	char allowed_chars[]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	
	char *ret=malloc(33);
	onion_random_generate(ret,32);
	int i;
	for (i=0;i<32;i++){
		ret[i]=allowed_chars[ ret[i]%(sizeof(allowed_chars)-1) ];
	}
	ret[i]='\0';
	return ret;
}

/**
 * @short Creates a sessions data object, which keeps all sessions in memory.
 * @memberof onion_sessions_t
 * 
 * TODO: Make it also to allow persistent storage: for example if sqlite is available.
 */
onion_sessions *onion_sessions_new(){
	onion_random_init();
	
	onion_sessions *ret=malloc(sizeof(onion_sessions));
	ret->sessions=onion_dict_new();
	return ret;
}

/// Helper to remove all dicts inside session. Called with onion_dict_preorder
static void onion_sessions_free_helper(void *data, const char *key, onion_dict *value, int flags){
	onion_dict_free(value);
}

/**
 * @short Frees the memory used by sessions
 * @memberof onion_sessions_t
 */
void onion_sessions_free(onion_sessions* sessions){
	onion_dict_preorder(sessions->sessions, (void*)onion_sessions_free_helper, NULL);
	onion_dict_free(sessions->sessions);
	free(sessions);

	onion_random_free();
}


/**
 * @short Creates a new session and returns the sessionId.
 * @memberof onion_sessions_t
 * 
 * @returns the name. Must be freed by user.
 */
char *onion_sessions_create(onion_sessions *sessions){
	char *sessionId=onion_sessions_generate_id();
	onion_dict *data=onion_dict_new();
	onion_dict_add(sessions->sessions, sessionId, data, OD_DUP_KEY|OD_DICT);
	ONION_DEBUG("Created the session '%s'",sessionId);
	return sessionId;
}

/**
 * @short Returns a session dictionary
 * @memberof onion_sessions_t
 * 
 * It returns a dupped (ref counter ++) version of the dict. All modifications are straight here, but user
 * (normally onion_request) must dereference it.. as it really do.
 * 
 * This is this way to prevent a thread removing the dict, whilst others are using it. to really remove the
 * dict, call onion_sessions_remove, which will remove the reference here, which is the only
 * one which should stay across thread lifetimes.
 * 
 * The sessionId is the session as asked by the client. If it does not exist it returns NULL, and 
 * onion_sessions_create has to be used. It used to reuse the sessionId if it doe snot exist, but that 
 * looks like an insecure pattern.
 * 
 * @returns The session for that id, or NULL if none.
 */
onion_dict *onion_sessions_get(onion_sessions *sessions, const char *sessionId){
	ONION_DEBUG0("Accessing session '%s'",sessionId);
	onion_dict *sess=onion_dict_get_dict(sessions->sessions, sessionId);
	if (!sess){
		ONION_DEBUG0("Unknown session '%s'.", sessionId);
		return NULL;
	}
	return onion_dict_dup(sess);
}

/**
 * @short Removes a session from the storage
 * @memberof onion_sessions_t
 */
void onion_sessions_remove(onion_sessions *sessions, const char *sessionId){
	onion_dict *data=onion_dict_get_dict(sessions->sessions, sessionId);
	if (data){
		onion_dict_free(data);
		onion_dict_remove(sessions->sessions, sessionId);
	}
}
