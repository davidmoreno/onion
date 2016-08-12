/** Licensed under AGPL 3.0. (C) 2016 David Moreno Montero. http://coralbits.com */
#include <onion.hpp>
#include <http.hpp>
#include <https.hpp>
#include <onion/log.h>

#include <shortcuts.hpp>

int main(int argc, char*argv[]){
	Onion::Onion o { O_THREADED };

	o.setRootHandler(Onion::ExportLocal("."));

	o.addListenPoint("localhost", "8080", Onion::HttpListenPoint());
	o.addListenPoint("localhost", "8081", Onion::HttpListenPoint());

#ifdef HAVE_GNUTLS
	Onion::HttpsListenPoint https;
	https.setCertificate(O_SSL_CERTIFICATE_KEY, "cert.pem", "cert.key");
	o.addListenPoint("localhost", "4443", std::move(https));
#else
	ONION_WARNING("HTTPS support is not enabled. Recompile with gnutls");
#endif

	o.listen();
	return 0;
}
