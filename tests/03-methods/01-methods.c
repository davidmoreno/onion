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

#include <string.h>

#include <onion/onion.h>
#include <onion/dict.h>
#include <onion/handler.h>
#include <onion/response.h>

void print_dict_element(onion_response *res, const char *key, const char *value, int flags){
	onion_response_write0(res,"<li> ");
	onion_response_write0(res,key);
	onion_response_write0(res," = ");
	onion_response_write0(res,value);
	onion_response_write0(res,"</li>\n");
}

onion_connection_status method(void *ignore, onion_request *req, onion_response *res){
	if (onion_response_write_headers(res)==OR_SKIP_CONTENT) // Head
		return OCS_PROCESSED;
	
	onion_response_write0(res, "<html><body>\n<h1>Petition resume</h1>\n");
	int flags=onion_request_get_flags(req);
	if (flags&OR_GET)
		onion_response_write0(res,"<h2>GET");
	else if (flags&OR_POST)
		onion_response_write0(res,"<h2>POST");
	else 
		onion_response_write0(res,"<h2>UNKNOWN");
	
	onion_response_printf(res," %s</h2>\n<ul>",onion_request_get_path(req));

	const onion_dict *get=onion_request_get_query_dict(req);
	const onion_dict *post=onion_request_get_post_dict(req);
	const onion_dict *headers=onion_request_get_header_dict(req);
	
	onion_response_printf(res,"<li>Header %d elements<ul>",onion_dict_count(headers));
	if (headers) onion_dict_preorder(headers, print_dict_element, res);
	onion_response_printf(res,"</ul></li>\n");

	onion_response_printf(res,"<li>GET %d elements<ul>",onion_dict_count(get));
	if (get) onion_dict_preorder(get, print_dict_element, res);
	onion_response_printf(res,"</ul></li>\n");

	onion_response_printf(res,"<li>POST %d elements<ul>",onion_dict_count(post));
	if (post) onion_dict_preorder(post, print_dict_element, res);
	onion_response_printf(res,"</ul></li>\n");

	onion_response_write0(res,"<p>\n");
	onion_response_write0(res,"<form method=\"GET\">"
														"<input type=\"text\" name=\"test\">"
														"<input type=\"submit\" name=\"submit\" value=\"GET\">"
														"</form><p>\n");
	onion_response_write0(res,"<form method=\"POST\" enctype=\"application/x-www-form-urlencoded\">"
														"<textarea name=\"text\"></textarea>"
														"<input type=\"text\" name=\"test\">"
														"<input type=\"submit\" name=\"submit\"  value=\"POST urlencoded\">"
														"</form>"
														"<p>\n");
	onion_response_write0(res,"<form method=\"POST\" enctype=\"multipart/form-data\">"
														"<input type=\"file\" name=\"file\">"
														"<textarea name=\"text\"></textarea>"
														"<input type=\"text\" name=\"test\">"
														"<input type=\"submit\" name=\"submit\" value=\"POST multipart\">"
														"</form>"
														"<p>\n");
	
	onion_response_write0(res,"</body></html>");
	
	return OCS_PROCESSED;
}

int main(int argc, char **argv){
	onion *onion=onion_new(O_ONE_LOOP);
	
	onion_handler *root=onion_handler_new((onion_handler_handler)method, NULL, NULL);

	onion_set_root_handler(onion, root);
	
	onion_listen(onion);
	
	return 0;
}
