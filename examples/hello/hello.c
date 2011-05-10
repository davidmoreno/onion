/** Licensed under AGPL 3.0. (C) 2010 David Moreno Montero. http://coralbits.com */
#include <onion/onion.h>

int hello(void *p, onion_request *req, onion_response *res){
	onion_response_write(res,"Hello world",11);
	return 1;
}

int main(int argc, char **argv){
	onion *o=onion_new(O_THREADED);
	onion_set_root_handler(o, onion_handler_new((void*)hello, NULL, NULL));
	onion_listen(o);
	onion_free(o);
	return 0;
}
