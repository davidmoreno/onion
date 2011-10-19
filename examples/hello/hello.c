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
	ONION_DEBUG("Proper close of hello.");
	if (o) 
		onion_listen_stop(o);
}

int main(int argc, char **argv){
	signal(SIGINT,shutdown);
	signal(SIGTERM,shutdown);
	
	o=onion_new(O_POOL);
	onion_set_timeout(o, 5000);
	onion_url *urls=onion_root_url(o);
	
	onion_url_add_static(urls, "static", "Hello static world", HTTP_OK);
	onion_url_add(urls, "", hello);
	onion_url_add(urls, "^(.*)$", hello);
	
	onion_listen(o);
	ONION_DEBUG("Ok, not listening anymore");
	onion_free(o);
	ONION_DEBUG("Finished OK");
	return 0;
}
