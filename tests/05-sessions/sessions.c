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

#include <onion/onion.h>
#include <onion/dict.h>
#include <onion/handler.h>
#include <onion/response.h>

void print_dict_element(const char *key, const char *value, onion_response *res){
	onion_response_write0(res,"<li> ");
	onion_response_write0(res,key);
	onion_response_write0(res," = ");
	onion_response_write0(res,value);
	onion_response_write0(res,"</li>\n");
}

onion_connection_status sessions(void *ignore, onion_request *req){
	onion_response *res=onion_response_new(req);
	onion_dict *session=onion_request_get_session_dict(req);

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
		return onion_response_free(res);
	
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
	
	return onion_response_free(res);
}

int main(int argc, char **argv){
	onion *onion=onion_new(O_ONE_LOOP);
	
	onion_handler *root=onion_handler_new((onion_handler_handler)sessions, NULL, NULL);

	onion_set_root_handler(onion, root);
	
	onion_listen(onion);
	
	return 0;
}
