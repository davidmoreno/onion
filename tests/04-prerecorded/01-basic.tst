GET / HTTP/1.1

-- --
Path: /
Method: GET
++ ++
UNKNOWN / HTTP/1.0

-- --
NOT_IMPLEMENTED
++ ++
GET /?test=1 HTTP/..

-- --
Path: /
Query: test = 1
++ ++
GET / HTTP/1.1
Connection: Close
Header-Name: Header-Value

-- --
Header: Connection = Close
Header: Header-Name = Header-Value
++ ++
POST /?get=true HTTP/1.1
Content-Type: post/urlencoded


-- --
INTERNAL_ERROR
++ ++
