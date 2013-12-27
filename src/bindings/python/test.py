from onion import Onion, O_POOL, O_POLL, OCS_PROCESSED
import json

def hello(request, response):
	d=request.header().copy()
	response.write(json.dumps(d))
	return OCS_PROCESSED

def bye(request, response):
	response.write_html_safe("<head>Bye!</head>")
	response.write_html_safe( 'path is <%s>'% request.fullpath() )
	response.write_html_safe( json.dumps( request.query().copy() ) )
	return OCS_PROCESSED

o=Onion(O_POOL)

urls=o.root_url()

urls.add("", hello)
urls.add_static("static", "This is static text")
urls.add(r"^(.*)", bye)

o.listen()
