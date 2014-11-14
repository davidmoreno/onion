/*
	Onion HTTP server library
	Copyright (C) 2011 David Moreno Montero

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

#include <signal.h>
#include <onion/log.h>
#include <onion/onion.h>
#include <onion/shortcuts.h>

/**
 * @short This handler just answers the content of the POST parameter "text"
 * 
 * It checks if its a HEAD in which case it writes the headers, and if not just printfs the
 * user data.
 */
onion_connection_status post_data(void *_, onion_request *req, onion_response *res){
	if (onion_request_get_flags(req)&OR_HEAD){
		onion_response_write_headers(res);
		return OCS_PROCESSED;
	}
	const char *user_data=onion_request_get_post(req,"text");
	onion_response_printf(res, "The user wrote: %s", user_data);
	return OCS_PROCESSED;
}

onion *o=NULL;

void onexit(int _){
	ONION_INFO("Exit");
	if (o)
		onion_listen_stop(o);
}

/**
 * This example creates a onion server and adds two urls: the base one is a static content with a form, and the
 * "data" URL is the post_data handler.
 */
int main(int argc, char **argv){
	o=onion_new(O_ONE_LOOP);
	onion_url *urls=onion_root_url(o);
	
	onion_url_add_static(urls, "", 
"<html>\n"
"<head>\n"
" <title>Simple post example</title>\n"
"</head>\n"
"\n"
"Write something: \n"
"<form method=\"POST\" action=\"data\">\n"
"<input type=\"text\" name=\"text\">\n"
"<input type=\"submit\">\n"
"</form>\n"
"\n"
"</html>\n", HTTP_OK);

	onion_url_add(urls, "data", post_data);

	signal(SIGTERM, onexit);	
	signal(SIGINT, onexit);	
	onion_listen(o);

	onion_free(o);
	return 0;
}
