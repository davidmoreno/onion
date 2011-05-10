/** Licensed under AGPL 3.0. (C) 2010 David Moreno Montero. http://coralbits.com */
#include <onion/onion.h>
#include <onion/handlers/static.h>

int hello(void *p, onion_request *req, onion_response *res){
	onion_response_set_length(res, 11);
	onion_response_write(res,"Hello world",11);
	return OCS_PROCESSED;
}

int main(int argc, char **argv){
	onion *o=onion_new(O_THREADED);
	onion_url *urls=onion_root_url(o);
	
	onion_url_add_handler(urls, "static", onion_handler_static("Hello static world", HTTP_OK));
	onion_url_add(urls, "^$", hello);
	
	onion_listen(o);
	onion_free(o);
	return 0;
}
