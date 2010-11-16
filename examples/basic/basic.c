/** Licensed undel AGPL 3.0. (C) 2010 David Moreno Montero */
#include <onion/onion.h>
#include <handlers/onion_handler_directory.h>
#include <handlers/onion_handler_auth_pam.h>

int main(int argc, char **argv){
	onion *o=onion_new(O_THREADED);
	if (onion_use_certificate(o, O_SSL_CERTIFICATE_KEY, "cert.pem", "cert.key")!=0) // No SSL, no fun
		return 1;
	onion_set_root_handler(o, onion_handler_auth_pam("Onion Example", "login", onion_handler_directory(".")));
	onion_listen(o);
	onion_free(o);
	return 0;
}
