Onion http server library
=========================

Onion plans to be a lightweight C library that facilitate to create simple HTTP servers. 

The use case is an existing application, or a new one, that needs some HTTP interconnection 
with the world. It uses the library to add some handlers for specific URLs and generate and 
serve the dynamic data as needed.

It also has security goals (SSL support) so that you just concentrate on what you want
to serve, and serve it.

Its not a web server per se, as it is not an executable.

If you wan tto compare to a web server, a web server would use a module or plugin to add 
some functionality. With libonion yo uhave the functionality and add the webserver as a plugin.