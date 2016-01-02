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

#include <stdio.h>
#include <string.h>
#include <signal.h>

#include <onion/onion.h>
#include <onion/dict.h>
#include <onion/log.h>
#include <onion/handlers/internal_status.h>

#include <onion/sessions_redis.h>


onion *o;

void free_onion(int unused){
	ONION_INFO("Closing connections");
	onion_free(o);
	exit(0);
}


void print_dict_element(onion_response *res, const char *key, const char *value, int flags){
	onion_response_write0(res,"<li> ");
	onion_response_write0(res,key);
	onion_response_write0(res," = ");
	onion_response_write0(res,value);
	onion_response_write0(res,"</li>\n");
}

onion_connection_status sessions(void *ignore, onion_request *req, onion_response *res){
	onion_dict *session=onion_request_get_session_dict(req);

	if (onion_request_get_query(req, "reset")){
		onion_request_session_free(req);
		onion_response_write0(res, "ok");
		return OCS_PROCESSED;
	}

	const char *n=onion_dict_get(session, "count");
	int count;
	if (n){
		count=atoi(n)+1;
	}
	else
		count=0;
	char tmp[16];
	snprintf(tmp,sizeof(tmp),"%d",count);
	onion_dict_add(session, "count", tmp, OD_DUP_ALL|OD_REPLACE);

	if (onion_response_write_headers(res)==OR_SKIP_CONTENT) // Head
		return OCS_PROCESSED;

	onion_response_write0(res, "<html><body>\n<h1>Session data</h1>\n");

	if (session){
		onion_response_printf(res,"<ul>\n");
		onion_dict_preorder(session, print_dict_element, res);
		onion_response_printf(res,"</ul>\n");
	}
	else{
		onion_response_printf(res,"No session data");
	}
	onion_response_write0(res,"</body></html>");

	return OCS_PROCESSED;
}

int main(int argc, char **argv){
	o=onion_new(O_ONE_LOOP);

#ifdef HAVE_REDIS
	onion_sessions *session_backend=onion_sessions_redis_new("localhost",6379);
	onion_set_session_backend(o, session_backend);
#endif

	onion_url *root=onion_root_url(o);

  onion_url_add_handler(root, "status",onion_internal_status());
  onion_url_add_handler(root, "^.*", onion_handler_new((onion_handler_handler)sessions, NULL, NULL));

	signal(SIGINT, free_onion);

	onion_listen(o);

	return 0;
}
