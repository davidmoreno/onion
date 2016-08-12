/** Licensed under AGPL 3.0. (C) 2010 David Moreno Montero. http://coralbits.com */
#include <onion/onion.h>
#include <onion/log.h>
#include <onion/version.h>
#include <signal.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>

int hello(void *p, onion_request *req, onion_response *res){
	//onion_response_set_length(res, 11);
	onion_response_write0(res,"Hello world");
	if (onion_request_get_query(req, "1")){
		onion_response_printf(res, "<p>Path: %s", onion_request_get_query(req, "1"));
	}
	onion_response_printf(res,"<p>Client description: %s",onion_request_get_client_description(req));
	return OCS_PROCESSED;
}

onion *o=NULL;

static void shutdown_server(int _){
	if (o)
		onion_listen_stop(o);
}

onion_connection_status random_timeout(void *_, onion_request *req, onion_response *res){
	int ms=2000 * ( ((float)RAND_MAX) / rand() );
	ONION_INFO("Wait %d ms", ms);
	usleep(ms * 1000);
	ONION_INFO("Done");
	onion_response_write(res,"OK",2);
	return OCS_PROCESSED;
}

int main(int argc, char **argv){
	signal(SIGINT,shutdown_server);
	signal(SIGTERM,shutdown_server);

	ONION_VERSION_IS_COMPATIBLE_OR_ABORT();

	o=onion_new(O_POOL);
	onion_set_timeout(o, 5000);
	onion_set_hostname(o,"0.0.0.0");
	onion_url *urls=onion_root_url(o);

	onion_url_add_static(urls, "static", "Hello static world", HTTP_OK);
	onion_url_add(urls, "timeout", random_timeout);
	onion_url_add(urls, "", hello);
	onion_url_add(urls, "^(.*)$", hello);

	onion_listen(o);
	onion_free(o);
	return 0;
}
