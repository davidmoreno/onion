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

#include <malloc.h>
#include <stdlib.h>
#include <time.h>

#include "sessions.h"
#include "types_internal.h"
#include "dict.h"
#include "log.h"

/**
 * @short Generates a unique id.
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
	int i;
	for (i=0;i<32;i++){
		int c=rand()%(sizeof(allowed_chars)-1);
		ret[i]=allowed_chars[c];
	}
	ret[i]='\0';
	return ret;
}

/**
 * @short Creates a sessions data object, which keeps all sessions in memory.
 * 
 * TODO: Make it also to allow persistent storage: for example if sqlite is available.
 */
onion_sessions *onion_sessions_new(){
	// Just in case nobody elses do it... If somebody else do it, then no problem.
	srand(time(NULL));
	
	onion_sessions *ret=malloc(sizeof(onion_sessions));
	ret->sessions=onion_dict_new();
	return ret;
}

/// Helper for preorder to remove all sessions
void onion_sessions_free_helper(const char *key, const char *value, void *p){
	onion_dict_free((onion_dict*)value);
}

/**
 * @short Frees the memory used by sessions
 */
void onion_sessions_free(onion_sessions* sessions){
	onion_dict_preorder(sessions->sessions, onion_sessions_free_helper, NULL);
	onion_dict_free(sessions->sessions);
	free(sessions);
}


/**
 * @short Creates a new session and returns the sessionId.
 * 
 * @returns the name. Must be freed by user.
 */
char *onion_sessions_create(onion_sessions *sessions){
	char *sessionId=onion_sessions_generate_id();
	onion_dict *data=onion_dict_new();
	onion_dict_add(sessions->sessions, sessionId, (char*)data, OD_DUP_KEY);
	ONION_DEBUG("Created the session '%s'",sessionId);
	return sessionId;
}

/**
 * @short Returns a session dictionary
 */
onion_dict *onion_sessions_get(onion_sessions *sessions, const char *sessionId){
	ONION_DEBUG("Accessing session '%s'",sessionId);
	onion_dict *sess=(onion_dict*)onion_dict_get(sessions->sessions, sessionId);
	if (!sess){
		ONION_DEBUG("Unknown session '%s'. Creating it.", sessionId);
		sess=onion_dict_new();
		onion_dict_add(sessions->sessions, sessionId, (char*)sess, OD_DUP_KEY);
	}
	return onion_dict_dup(sess);
}

/**
 * @short Removes a session from the storage
 */
void onion_sessions_remove(onion_sessions *sessions, const char *sessionId){
	onion_dict *data=(onion_dict*)onion_dict_get(sessions->sessions, sessionId);
	if (data){
		onion_dict_free(data);
		onion_dict_remove(sessions->sessions, sessionId);
	}
}
