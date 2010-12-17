Introduction
------------

Utility to make prerecorded petitions to a fake server. 

It executed all passed arguments as petitions to the server, that is just a formatter of headers, post, gets... and checks it returns a given line, 
allowing regular expressions.


Petition format
---------------

The petitions have the following format::

 Petition
 -- --
 Regular expression list.
 ++ ++
 Petition 2
 -- --
 regular expresison list 2.

For example:

 GET / HTTP/1.1

 -- --
 Path: /
 ++ ++
 GET /?test=1
 Test-Header: Test
 
 -- --
 Header: Test-Header: Test
 GET: test=[0-9]
 ++ ++
 UNKONWN / HTTP/0.9

 -- --
 500 Internal error


It performs the tests both with \n and \r\n.

