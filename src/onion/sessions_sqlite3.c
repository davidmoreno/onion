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
#include <string.h>
#include <sqlite3.h>
#ifdef HAVE_PTHREADS
# include <pthread.h>
#endif

#include "sessions.h"
#include "sessions_sqlite3.h"
#include "types_internal.h"
#include "dict.h"
#include "random.h"
#include "block.h"
#include "log.h"
#include "low.h"

typedef struct onion_session_sqlite3_t{
	sqlite3 *db;
	sqlite3_stmt *save;
	sqlite3_stmt *get;
#ifdef HAVE_PTHREADS
	pthread_mutex_t mutex;
#endif
} onion_session_sqlite3;


static void onion_sessions_sqlite3_free(onion_sessions *sessions)
{
	ONION_DEBUG0("Free onion sessions sqlite3");
	onion_session_sqlite3 *p=sessions->data;

#ifdef HAVE_PTHREADS
	pthread_mutex_destroy(&p->mutex);
#endif
	sqlite3_finalize(p->save);
	sqlite3_finalize(p->get);
	sqlite3_close(p->db);
	onion_low_free(sessions->data);

	onion_low_free(sessions);
}

static onion_dict* onion_sessions_sqlite3_get(onion_sessions *sessions, const char *session_id){
	onion_session_sqlite3 *p=sessions->data;
	ONION_DEBUG0("Load session %s", session_id);
	onion_dict *ret=NULL;

#ifdef HAVE_PTHREADS
	pthread_mutex_lock(&p->mutex);
#endif
	sqlite3_reset(p->get);
	int rc=sqlite3_bind_text(p->get, 1, session_id, -1, SQLITE_STATIC);
	if( rc!=SQLITE_OK ){
		ONION_ERROR("Error binding session_id");
		goto exit;
	}
	rc=sqlite3_step(p->get);
	if( rc==SQLITE_DONE ){
		ONION_DEBUG0("No session found. Returning NULL");
		goto exit;
	}
	else if( rc!=SQLITE_ROW ){
		ONION_ERROR("Error loading session (%d)", rc);
		goto exit;
	}
	const char * text;
	text  = (const char *)sqlite3_column_text (p->get, 0);

	ret=onion_dict_from_json(text);
	if (!ret){
		ONION_DEBUG0("Invalid session parsing for id %s: %s", session_id, text);
		ONION_ERROR("Invalid session data");
	}
exit:
#ifdef HAVE_PTHREADS
	pthread_mutex_unlock(&p->mutex);
#endif

	return ret;
}

void onion_sessions_sqlite3_save(onion_sessions *sessions, const char *session_id, onion_dict *data){
	onion_block *bl=onion_dict_to_json(data);

	ONION_DEBUG0("Save session %s: %s", session_id, onion_block_data(bl));
	const char *json=onion_block_data(bl);

	int rc;
	onion_session_sqlite3 *p=sessions->data;
#ifdef HAVE_PTHREADS
	pthread_mutex_lock(&p->mutex);
#endif
	sqlite3_reset(p->save);
	rc=sqlite3_bind_text(p->save, 1, session_id, -1, SQLITE_STATIC);
	if( rc!=SQLITE_OK ){
		ONION_ERROR("Error binding session_id");
		goto error;
	}
	rc=sqlite3_bind_text(p->save, 2, json, -1, SQLITE_STATIC);
	if( rc!=SQLITE_OK ){
		ONION_ERROR("Error binding json data");
		goto error;
	}
	rc=sqlite3_step(p->save);
	if( rc!=SQLITE_DONE ){
		ONION_ERROR("Error saving session (%d)", rc);
		goto error;
	}

	ONION_DEBUG0("Session saved");
error:
	onion_block_free(bl);
#ifdef HAVE_PTHREADS
	pthread_mutex_unlock(&p->mutex);
#endif
	return;
}

/**
 * @short Creates a sqlite backend for sessions
 * @ingroup sessions
 *
 * @see onion_set_session_backend
 */
onion_sessions* onion_sessions_sqlite3_new(const char *database_filename)
{
	onion_random_init();

	int rc;
	sqlite3 *db;
	sqlite3_stmt *save;
	sqlite3_stmt *get;
	rc = sqlite3_open(database_filename, &db);
	if( rc ){
		ONION_ERROR("Can't open database: %s", sqlite3_errmsg(db));
		sqlite3_close(db);
		return NULL;
	}
	// I blindly try to create tables. If they exist, error, if not, create
	sqlite3_exec(db, "CREATE TABLE sessions (id TEXT PRIMARY KEY, data TEXT)", 0,0,0);

	const char *GET_SQL="SELECT data FROM sessions WHERE id=?";
	rc=sqlite3_prepare_v2(db, GET_SQL, strlen(GET_SQL), &get, NULL);
	if( rc != SQLITE_OK ){
		ONION_ERROR("Cant prepare statement to get (%d)", rc);
		sqlite3_close(db);
		return NULL;
	}

	const char *SAVE_SQL="INSERT OR REPLACE INTO sessions (id, data) VALUES (?, ?);";
	rc=sqlite3_prepare_v2(db, SAVE_SQL, strlen(SAVE_SQL), &save, NULL);
	if( rc != SQLITE_OK ){
		ONION_ERROR("Cant prepare statement to save (%d)", rc);
		sqlite3_close(db);
		return NULL;
	}

	onion_sessions *ret=onion_low_malloc(sizeof(onion_sessions));

	ret->data=onion_low_malloc(sizeof(onion_session_sqlite3));
	ret->free=onion_sessions_sqlite3_free;
	ret->get=onion_sessions_sqlite3_get;
	ret->save=onion_sessions_sqlite3_save;

	onion_session_sqlite3 *p=ret->data;
	p->db=db;
	p->save=save;
	p->get=get;
#ifdef HAVE_PTHREADS
	pthread_mutex_init(&p->mutex, NULL);
#endif

	return ret;
}
