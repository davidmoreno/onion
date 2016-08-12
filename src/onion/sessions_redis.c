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
#include <hiredis/hiredis.h>
#ifdef HAVE_PTHREADS
# include <pthread.h>
#endif

#include "sessions.h"
#include "sessions_redis.h"
#include "types_internal.h"
#include "dict.h"
#include "random.h"
#include "block.h"
#include "log.h"
#include "low.h"

typedef struct onion_session_redis_t {
	redisContext* context;
#ifdef HAVE_PTHREADS
	pthread_mutex_t mutex;
#endif
} onion_session_redis;

static void onion_sessions_redis_free(onion_sessions *sessions)
{
	ONION_DEBUG0("Free onion sessions redis");
	onion_session_redis *p = sessions->data;

#ifdef HAVE_PTHREADS
	pthread_mutex_destroy(&p->mutex);
#endif

	redisFree(p->context);
	onion_low_free(sessions->data);
	onion_low_free(sessions);
}

static onion_dict* onion_sessions_redis_get(onion_sessions* sessions, const char *session_id)
{
	onion_session_redis *p = sessions->data;
	ONION_DEBUG0("Load session %s", session_id);
	onion_dict *ret = NULL;

#ifdef HAVE_PTHREADS
	pthread_mutex_lock(&p->mutex);
#endif

        // When commands are sent via redisCommand, they are interpolated by the library
        // so it will avoid any type of command injection. No need to worry about sending
        // the session_id directly to redis.
	redisReply* reply = redisCommand(p->context, "HEXISTS SESSIONS %b", session_id, strlen(session_id));

	if(reply->type != REDIS_REPLY_INTEGER)
	{
		ONION_ERROR("Error getting session_id");
		freeReplyObject(reply);
		goto exit;
	}
	else
	{
		if(reply->integer == 1)
		{
			freeReplyObject(reply);
			reply = redisCommand(p->context, "HGET SESSIONS %b", session_id, strlen(session_id));

			if(reply->type != REDIS_REPLY_STRING)
			{
				ONION_ERROR("Error loading session.");
				freeReplyObject(reply);
				goto exit;
			}
			else
			{
				ret = onion_dict_from_json(reply->str);
				freeReplyObject(reply);
				goto exit;
			}
		}
		else
		{
			ONION_DEBUG0("No session found. Returning NULL");
			freeReplyObject(reply);
			goto exit;
		}
	}
exit:
#ifdef HAVE_PTHREADS
	pthread_mutex_unlock(&p->mutex);
#endif
	return ret;
}

void onion_sessions_redis_save(onion_sessions *sessions, const char *session_id, onion_dict* data)
{
	onion_block *bl = onion_dict_to_json(data);

	ONION_DEBUG0("Save session %s: %s", session_id, onion_block_data(bl));

	onion_session_redis *p = sessions->data;
#ifdef HAVE_PTHREADS
	pthread_mutex_lock(&p->mutex);
#endif

	if(p == NULL)
	{
		redisReply* reply = redisCommand(p->context, "HDEL SESSIONS %b", session_id, strlen(session_id));

		if(reply->type != REDIS_REPLY_INTEGER)
		{
			ONION_ERROR("Error removing session");
		}
		freeReplyObject(reply);
	} else
    {
		const char* json = onion_block_data(bl);
		redisReply* reply = redisCommand(p->context, "HSET SESSIONS %b %b", session_id, strlen(session_id), json, strlen(json));

		if(reply->type != REDIS_REPLY_INTEGER)
		{
			ONION_ERROR("Error saving session");
		}
		freeReplyObject(reply);
		onion_block_free(bl);
	}

#ifdef HAVE_PTHREADS
	pthread_mutex_unlock(&p->mutex);
#endif
	return;
}

/**
 * @short Creates a redis backend for sessions
 * @ingroup sessions
 */
onion_sessions* onion_sessions_redis_new(const char* server_ip, int port)
{
	onion_random_init();

	onion_sessions *ret = onion_low_malloc(sizeof(onion_sessions));
	ret->data = onion_low_malloc(sizeof(onion_session_redis));
	ret->free=onion_sessions_redis_free;
	ret->get=onion_sessions_redis_get;
	ret->save=onion_sessions_redis_save;

	onion_session_redis *p = ret->data;
	p->context = redisConnect(server_ip, port);

	if(p->context != NULL && p->context->err)
	{
		ONION_ERROR("Can't connect to redis. Error (%s)", p->context->errstr);
		redisFree(p->context);
		return NULL;
	}

#ifdef HAVE_PTHREADS
	pthread_mutex_init(&p->mutex, NULL);
#endif

	return ret;
}
