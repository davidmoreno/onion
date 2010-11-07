Onion http server library
=========================

Onion plans to be a lightweight C library that facilitate to create simple HTTP servers. 

The use case is an existing application, or a new one, that needs some HTTP interconnection 
with the world. It uses the library to add some handlers for specific URLs and generate and 
serve the dynamic data as needed.

It also has security goals (SSL support) so that you just concentrate on what you want
to serve, and serve it.

Its not a web server per se, as it is not an executable.

If you want to compare to a web server, a web server would use a module or plugin to add 
some functionality. With libonion you have the functionality and add the webserver as a plugin.


SSL Support
-----------

If at compile time it finds the gnutls libraries, SSL support is compiled in. It can be 
deactivated anyway at ./CMakeLists.txt. 

To use it you have to set the certificates, and you can check if its on, checking the flag
O_SSL_ACTIVATED.

If support is not in, then the library will not use, but for the user of the library the
interface is the same; it will only change that when trying to set the certificates it 
will fail. Anwyay for clients its just to use the interface and they dont care at all
if suport is in or not. No more than beign able to use SSL.

this is this way, and not mandatory as ther may be moments where the program user do not
want to support SSL for whatever reasons, for example speed.
