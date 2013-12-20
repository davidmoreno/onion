#include <onion/onion.h>
#include <onion/handlers/webdav.h>


int main(void){
	onion *o=onion_new(O_POOL);
	
	onion_url *root=onion_root_url(o);
	onion_url_add_handler(root, "^wd", onion_handler_webdav(".",NULL));
	
	onion_listen(o);
	
	return 0;
}
