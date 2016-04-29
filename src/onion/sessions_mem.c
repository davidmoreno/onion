/*
	Onion HTTP server library
	Copyright (C) 2010-2016 David Moreno Montero and others

	This library is free software; you can redistribute it and/or
	modify it under the terms of, at your choice:

	a. the Apache License Version 2.0.

	b. the GNU General Public License as published by the
		Free Software Foundation; either version 2.0 of the License,
		or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of both libraries, if not see
	<http://www.gnu.org/licenses/> and
	<http://www.apache.org/licenses/LICENSE-2.0>.
	*/

#include <stdlib.h>

#include "sessions_mem.h"
#include "types_internal.h"
#include "dict.h"
#include "log.h"
#include "random.h"
#include "low.h"

char *last_data=NULL;

static onion_dict *onion_sessions_mem_get(onion_sessions *sessions, const char *session_id){
	ONION_DEBUG0("Accessing session '%s'",session_id);
	onion_dict *sess=onion_dict_get_dict(sessions->data, session_id);
	if (!sess){
		ONION_DEBUG0("Unknown session '%s'.", session_id);
		return NULL;
	}
	return onion_dict_dup(sess);
}

static void onion_sessions_mem_save(onion_sessions *sessions, const char *session_id, onion_dict *data){
	if (data==NULL){
		data=onion_dict_get_dict(sessions->data, session_id);
		if (data){
			onion_dict_remove(sessions->data, session_id);
		}
		return;
	}
	onion_dict_add(sessions->data, session_id, onion_dict_dup(data), OD_DUP_KEY|OD_FREE_VALUE|OD_DICT|OD_REPLACE);
}

static void onion_sessions_mem_free(onion_sessions *sessions){
	onion_dict_free(sessions->data);
	onion_low_free(sessions);
}

/**
 * @short Creates a inmem backend for sessions
 * @ingroup sessions
 *
 * This is the default and less interesting of the session backends. 
 *
 * @see onion_set_session_backend
 */
onion_sessions *onion_sessions_mem_new(){
	onion_random_init();

	onion_sessions *ret=onion_low_malloc(sizeof(onion_sessions));
	ret->data=onion_dict_new();

	ret->get=onion_sessions_mem_get;
	ret->save=onion_sessions_mem_save;
	ret->free=onion_sessions_mem_free;

	return ret;
}
