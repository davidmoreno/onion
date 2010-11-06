Onion web-based GNU/Linux terminal
==================================

With Oterm you can acces with a standard compilant browser to a remote terminal, 
using SSL and authenticated.

It has a server component, that is as light as possible, that is the container of 
the processes, and redirects all inforamtion to the web based terminal.

The terminal uses HTML/Javascript to emulate all the terminal commands and 
control characters, and serves all that as if it were a web page.

Usage
-----

You need the libonion http web server library, and compile it all. To execute it 
just run the command oterm, and connect to the given address. 

More help is provided with --help.

Current features
----------------

* Understand most linux terminal commands, and allow to use many terminal based 
  programs as htop, atop, mc, vi, emacs

Planned features
----------------

* Make it behave like a screen for web, allowing to create new sessions, recover 
  old ones...
* Make it multiuser, each with its own sessions.
* Make it RESTfull, now it does polling, so its not as efficient as could be.
* More terminal functionalities: It does not do yet all that a xterm do, for example 
  it does not resize yet.
* Fix bugs. 

Contact
-------

There is a github page at https://github.com/davidmoreno/onion, where you can fill new
issues and feature request. Also you can send emails to dmoreno (works at) coralbits.com.

Last if you need prefesional guidance about oterm or onion, you can contact the upper
email address to ask for more information about consulting.
