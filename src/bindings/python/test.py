from onion import Onion, O_POOL, OCS_PROCESSED
import json

def hello(request, response):
	response.write(json.dumps(dict(
		data="Hello world"
		)))
	return OCS_PROCESSED

def bye(request, response):
	response.write("Bye!")
	return OCS_PROCESSED

o=Onion(O_POOL)

urls=o.root_url()

urls.add("", hello)
urls.add("^.*", bye)

o.listen()
