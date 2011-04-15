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

API documentation is at http://coralbits.com/staticc/libonion/html/.

SSL Support
-----------

If at compile time it finds the gnutls libraries, SSL support is compiled in. It can be 
deactivated anyway at ./CMakeLists.txt. 

To use it you have to set the certificates, and you can check if its on, checking the flag
O_SSL_ACTIVATED.

If support is not in, then the library will not use, but for the user of the library the
interface is the same; it will only change that when trying to set the certificates it 
will fail. Anwyay for clients its just to use the interface and they dont care at all
if suport is in or not. No more than being able to use SSL.

this is this way, and not mandatory as ther may be moments where the program user do not
want to support SSL for whatever reasons, for example speed.


Threads support
---------------

Currently there is basic threads support. It can be set the server to be created as 
threaded (O_THREADED), and it will create a new thread per connection. There is no
data protection as on the listen phase there should not be any change to onion structures.

Nevertheless if new handlers are created they must set their own threading support
as necesary.

It can be deactivated at CMakeLists.txt. If no pthreads lib is found on the system, it
is not compiled in.

Also when thread support is on, onion server can set to work on another (non-main) thread. 
This is independant from O_THREADED operation; it can have one thread with your normal 
application and another thread that listens and processes petitions. Its set with the 
O_DETACH_LISTEN flag. This is very useful when adding an extra web server to your application
as it can be added without changes to the flow of your application, but you will need to
thread protect your data if you access to it from the web server.


ARM Support
-----------

It can be cross compiled for ARM directly from cmake. Just do:

		$ mkdir arm
		$ cd arm
		$ cmake .. -DARM=yes
		$ make

Tested on ubuntu 10.10, with gcc-4.5-arm-linux-gnueabi installed.


Templating support
------------------

Starting on 0.3.0 development onion has templating support via otemplate. It is a template
system similar to django templates (http://docs.djangoproject.com/en/dev/topics/templates/).

Check more information on how to use them at tools/otemplate/README.rst.

