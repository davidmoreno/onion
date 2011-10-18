/** Licensed under AGPL 3.0. (C) 2010 David Moreno Montero. http://coralbits.com */
#include <onion/onion.h>
#include <onion/log.h>
#include <signal.h>

int hello(void *p, onion_request *req, onion_response *res){
	//onion_response_set_length(res, 11);
	onion_response_write0(res,"Hello world");
	if (onion_request_get_query(req, "1")){
		onion_response_printf(res, "<p>Path: %s", onion_request_get_query(req, "1"));
	}
	return OCS_PROCESSED;
}

onion *o=NULL;

static void shutdown(int _){
	if (o) onion_free(o);
	exit(0);
}

int main(int argc, char **argv){
	signal(SIGINT,shutdown);
	signal(SIGTERM,shutdown);
	
	onion_set_timeout(o, 5000);
	onion *o=onion_new(O_THREADED|O_POLL);
	onion_url *urls=onion_root_url(o);
	
	onion_url_add_static(urls, "static", "Hello static world", HTTP_OK);
	onion_url_add(urls, "", hello);
	onion_url_add(urls, "^(.*)$", hello);
	
	onion_listen(o);
	onion_free(o);
	return 0;
}
