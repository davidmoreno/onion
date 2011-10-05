#include <onion/onion.h>
#include <onion/shortcuts.h>

onion_connection_status post_data(void *_, onion_request *req, onion_response *res){
	onion_response_write_headers(res);
	if (onion_request_get_flags(req)&OR_HEAD)
		return OCS_PROCESSED;
	const char *user_data=onion_request_get_post(req,"text");
	onion_response_printf(res, "The user wrote: %s", user_data);
	return OCS_PROCESSED;
}

int main(int argc, char **argv){
	onion *o=onion_new(O_ONE_LOOP);
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
	
	onion_listen(o);
}
