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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <onion/log.h>
#include <onion/onion.h>
#include <onion/dict.h>
#include <onion/handler.h>
#include <onion/request.h>
#include <onion/response.h>

/**
 * @short Asks a question and returns it.
 * 
 * The returned pointer must be freed by user.
 */
char *ask_question(const char *question){
	printf("Answer for path '%s': ",question);
	char *ret=calloc(1, 1024);
	if (!fgets(ret, 1024, stdin)){
		free(ret);
		return NULL;
	}
	return ret;
}

/// Format query string a little bit to understand the query itself
void format_query(const char *key, const char *value, char *temp){
	strcat(temp," ");
	strcat(temp,key);
	strcat(temp,"=");
	strcat(temp,value);
}

/**
 * @short Just asks the user for the answer.
 */
onion_connection_status ask_handler(void *none, onion_request *req, onion_response *res){
	char temp[1024];
	strncpy(temp, onion_request_get_path(req), sizeof(temp)-1);
	onion_dict_preorder(onion_request_get_query_dict(req),format_query,temp);
	
	char *resp=ask_question(temp);
	
	if (!resp)
		return OCS_INTERNAL_ERROR;
	
	onion_response_write0(res, resp);
	free(resp);
	return OCS_PROCESSED;
}

/**
 * @short In a simple loop asks the user for the answer of the query.
 */
int main(int argc, char **argv){
	onion *server;
	
	server=onion_new(O_ONE_LOOP);
	
	onion_set_hostname(server, "0.0.0.0");
	onion_set_root_handler(server, onion_handler_new((void*)ask_handler, NULL, NULL));
	
	ONION_INFO("Listening at http://0.0.0.0:8080");
	onion_listen(server);
	
	onion_free(server);
	
	return 0;
}
