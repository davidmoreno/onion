/** Licensed under AGPL 3.0. (C) 2010 David Moreno Montero. http://coralbits.com */
#include <onion/onion.h>

#include <onion/handlers/exportlocal.h>
#include <onion/handlers/auth_pam.h>

int main(int argc, char **argv){
	onion *o=onion_new(O_THREADED);
	onion_set_certificate(o, O_SSL_CERTIFICATE_KEY, "cert.pem", "cert.key", O_SSL_NONE);
	onion_set_root_handler(o, onion_handler_auth_pam("Onion Example", "login", onion_handler_export_local_new(".")));
	onion_listen(o);
	onion_free(o);
	return 0;
}
