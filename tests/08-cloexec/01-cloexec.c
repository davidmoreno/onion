#include <onion/onion.h>



int handler(void *_, onion_request *req, onion_response *res){
	int ok=system("./testfds 0 1 2");
	
	if (ok==0){
		onion_response_write0(res, "OK");
	}
	else{
		onion_response_set_code(res, HTTP_FORBIDDEN);
		onion_response_write0(res, "ERROR. Did not close everything!");
	}
	return OCS_PROCESSED;
}

/**
 * @short Simple server that always launch testfds, and returns the result.
 */
int main(int argc, char **argv){
	onion *o;
	
	o=onion_new(O_ONE_LOOP);
	
	onion_url *urls=onion_root_url(o);
	onion_url_add(urls, "^.*", handler);
	
	return onion_listen(o);
}
