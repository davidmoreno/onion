POST /?get=true HTTP/1.1
Content-Type: application/x-www-form-urlencoded
Content-Length: 0


-- --
Path: /
Query: get = true
Method: POST
++ ++
GET /CaptureSetup/CapturePrivileges HTTP/1.1
Host: wiki.wireshark.org
Connection: keep-alive
Referer: http://www.google.es/search?sourceid=chrome&ie=UTF-8&q=wireshark+as+user
Cache-Control: max-age=0
Accept: application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png,*/*;q=0.5
User-Agent: Mozilla/5.0 (X11; U; Linux x86_64; en-US) AppleWebKit/534.10 (KHTML, like Gecko) Chrome/8.0.552.224 Safari/534.10
Accept-Encoding: gzip,deflate,sdch
Accept-Language: es-ES,es;q=0.8
Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3
Cookie: __utma=44101410.140457075.1292935187.1292935187.1292935187.1; __utmb=44101410; __utmc=44101410; __utmz=44101410.1292935187.1.1.utmccn=(organic)|utmcsr=google|utmctr=wireshark+as+user|utmcmd=organic

-- --
Method: GET
Path: /CaptureSetup/CapturePrivileges
Version: HTTP/1.1
++ ++
POST / HTTP/1.1
Host: localhost:8080
Connection: keep-alive
Referer: http://localhost:8080/
Content-Length: 34
Cache-Control: max-age=0
Origin: http://localhost:8080
Content-Type: application/x-www-form-urlencoded
Accept: application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png,*/*;q=0.5
User-Agent: Mozilla/5.0 (X11; U; Linux x86_64; en-US) AppleWebKit/534.10 (KHTML, like Gecko) Chrome/8.0.552.224 Safari/534.10
Accept-Encoding: gzip,deflate,sdch
Accept-Language: es-ES,es;q=0.8
Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3
Cookie: sessionid=59bf842eb99b88207aab82e90314f367; csrftoken=3737b812bde8e9e7eb5a5b4e10eba90f

text=&test=&submit=POST+urlencoded
-- --
POST: text = 
POST: test = 
POST: submit = POST urlencoded
++ ++
POST / HTTP/1.1
Host: localhost:8080
Connection: keep-alive
Referer: http://localhost:8080/
Content-Length: 437
Cache-Control: max-age=0
Origin: http://localhost:8080
Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryAwobIbqAyMCaZmpO
Accept: application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png,*/*;q=0.5
User-Agent: Mozilla/5.0 (X11; U; Linux x86_64; en-US) AppleWebKit/534.10 (KHTML, like Gecko) Chrome/8.0.552.224 Safari/534.10
Accept-Encoding: gzip,deflate,sdch
Accept-Language: es-ES,es;q=0.8
Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3
Cookie: sessionid=59bf842eb99b88207aab82e90314f367; csrftoken=3737b812bde8e9e7eb5a5b4e10eba90f

------WebKitFormBoundaryAwobIbqAyMCaZmpO
Content-Disposition: form-data; name="file"; filename=""


------WebKitFormBoundaryAwobIbqAyMCaZmpO
Content-Disposition: form-data; name="text"


------WebKitFormBoundaryAwobIbqAyMCaZmpO
Content-Disposition: form-data; name="test"


------WebKitFormBoundaryAwobIbqAyMCaZmpO
Content-Disposition: form-data; name="submit"

POST multipart
------WebKitFormBoundaryAwobIbqAyMCaZmpO--
-- --
POST: text = 
POST: test = 
POST: submit = POST multipart
++ ++
POST / HTTP/1.1
Host: localhost:8080
Connection: keep-alive
Referer: http://localhost:8080/
Content-Length: 3006
Cache-Control: max-age=0
Origin: http://localhost:8080
Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryWD5dWHBwOQploGj9
Accept: application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png,*/*;q=0.5
User-Agent: Mozilla/5.0 (X11; U; Linux x86_64; en-US) AppleWebKit/534.10 (KHTML, like Gecko) Chrome/8.0.552.224 Safari/534.10
Accept-Encoding: gzip,deflate,sdch
Accept-Language: es-ES,es;q=0.8
Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3
Cookie: sessionid=59bf842eb99b88207aab82e90314f367; csrftoken=3737b812bde8e9e7eb5a5b4e10eba90f

------WebKitFormBoundaryWD5dWHBwOQploGj9
Content-Disposition: form-data; name="file"; filename="README.rst"

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

------WebKitFormBoundaryWD5dWHBwOQploGj9
Content-Disposition: form-data; name="text"

README
------WebKitFormBoundaryWD5dWHBwOQploGj9
Content-Disposition: form-data; name="test"

README
------WebKitFormBoundaryWD5dWHBwOQploGj9
Content-Disposition: form-data; name="submit"

POST multipart
------WebKitFormBoundaryWD5dWHBwOQploGj9--
-- --
POST: test = README
POST: submit = POST multipart
POST: file = README.rst
FILE: file = .*
++ ++
POST / HTTP/1.1
Host: localhost:8080
Connection: keep-alive
Referer: http://localhost:8080/
Content-Length: 4
Cache-Control: max-age=0
Origin: http://localhost:8080
Content-Type: application/x-www-form-urlencoded
Accept: application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png,*/*;q=0.5
User-Agent: Mozilla/5.0 (X11; U; Linux x86_64; en-US) AppleWebKit/534.10 (KHTML, like Gecko) Chrome/8.0.552.224 Safari/534.10
Accept-Encoding: gzip,deflate,sdch
Accept-Language: es-ES,es;q=0.8
Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3
Cookie: sessionid=59bf842eb99b88207aab82e90314f367; csrftoken=3737b812bde8e9e7eb5a5b4e10eba90f

text=&test=&submit=POST+urlencoded
-- --
!POST:.*
Header:.*
++ ++
POST / HTTP/1.1
Host: localhost:8080
Connection: keep-alive
Referer: http://localhost:8080/
Content-Length: 6
Cache-Control: max-age=0
Origin: http://localhost:8080
Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryWD5dWHBwOQploGj9
Accept: application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png,*/*;q=0.5
User-Agent: Mozilla/5.0 (X11; U; Linux x86_64; en-US) AppleWebKit/534.10 (KHTML, like Gecko) Chrome/8.0.552.224 Safari/534.10
Accept-Encoding: gzip,deflate,sdch
Accept-Language: es-ES,es;q=0.8
Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3
Cookie: sessionid=59bf842eb99b88207aab82e90314f367; csrftoken=3737b812bde8e9e7eb5a5b4e10eba90f

------WebKitFormBoundaryWD5dWHBwOQploGj9
Content-Disposition: form-data; name="file"; filename="README.rst"

Onion http server library
=========================

Onion plans to be a lightweight C library that facilitate to create simple HTTP servers. 
-- --
INTERNAL_ERROR
