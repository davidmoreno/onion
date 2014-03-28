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

There is a wiki available at https://github.com/davidmoreno/onion/wiki, with many useful 
information on how to start and internal workings.

API documentation is at http://coralbits.com/static/onion/.

There is a mailing list at https://groups.google.com/a/coralbits.com/forum/?fromgroups=#!forum/onion-dev

Colaborate!
-----------

You can, and are encouraged, to branch at github, download and tweak onion to use it in your 
projects.

The library is under the LGPL license, so you can make almost everything with it, use it
in your commercial and free software programs, and modify it to your needs.

Please join the mailing list at https://groups.google.com/a/coralbits.com/group/onion-dev/topics,
to ask your questions and comment on your success using onion.

There is also a blog to keep everybody informed about news on onion at http://blog.coralbits.com/.

Compile and Install
-------------------

Manual compile and install:

    ::

     $ git clone git@github.com:davidmoreno/onion.git
     $ cd onion
     $ mkdir build
     $ cd build
     $ cmake ..
     $ make
     $ sudo make install
     
Dependencies
------------

Required:

* C compiler
* cmake
* make
 
Optional; Onion will compile but some functionality will not be available:

* gnutls
* pthreads
* libxml2
* libpam
* C++ compiler
* Systemd

Optional for examples:

* cairo
* libpng2

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

Currently there are two threads modes. It can be set the server to be created as 
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

Finally there is a pool mode. User can set a default number of threads (onion_set_max_threads), 
and using epoll the data is given to the threads. This is the highest performant method, with
up to 30k petitions served on a Intel(R) Core(TM)2 Duo CPU T6500  @2.10GHz.


ARM Support
-----------

It can be cross compiled for ARM directly from cmake. Just do:
    
    	::
    	
	$ mkdir arm
	$ cd arm
	$ cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchain/arm.txt
	$ make

It needs the current system opack and otemplate to compile some examples, so if you want to use
the examples on your instalation, compile and install libonion for the current system first.

Tested on ubuntu 10.10, with gcc-4.5-arm-linux-gnueabi and g++-4.5-arm-linux-gnueabi installed.


Templating support
------------------

Starting on 0.3.0 development onion has templating support via otemplate. It is a template
system similar to django templates (http://docs.djangoproject.com/en/dev/topics/templates/).

Check more information on how to use them at tools/otemplate/README.rst.

I18N
----

There is I18N support. Check wiki for details or fileserver_otemplate example.

Systemd
-------

Systemd is integrated. If want to use it, just pass the flag O_SYSTEMD to the onion_new().

Oterm has example socket and service files for oterm support.

FreeBSD/Darwin
--------------

Since september 2013 there is support for FreeBSD using libev or libevent. This work is not as tested 
as the Linux version, but if some compilation error arises, please send the bug report and we will fix
it ASAP.

OSX/Darwin support is also available on the darwin branch.

Once this work stabilizes it will be merged back to master.

Environmental variables
-----------------------

You can set the following envvars to modify runtime behaviour of onion:

* ONION_LOG

  - noinfo   -- Disables all info output to the console, to achieve faster results
  - nocolor  -- Disable color use by the log
  - nodebug  -- Do not show debug lines
  - syslog   -- Log to syslog. Can be changed programatically too, with the onion_log global function.

* ONION_DEBUG0   -- Set the filename of a c source file, and DEBUG0 log messages are written. This is normally very verbose.
* ONION_SENDFILE -- Set to 0 do disable sendfile. Under some file systems it does not work. Until a detection code is in place, it can be disabled with this.

Binary compatibility breaks
---------------------------

We try hard to keep binary compatibility, but sometimes its hard. Here is a list of ABI breaks:

>0.4.0 
''''''

* Onion object private flags have moved. If on your code you rely on them, must recompile. If 
  dont rely on them, everything should keep working.

.. image:: https://cruel-carlota.pagodabox.com/e788af315b3d9517752db2e79553e346
  :alt: Analytics.

