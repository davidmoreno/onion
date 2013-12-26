from . import _libonion, OCS_INTERNAL_ERROR
from request import Request
from response import Response
from ctypes import CFUNCTYPE, c_int, c_void_p

class Url(object):
	CBFUNC=CFUNCTYPE(c_int, c_void_p, c_void_p, c_void_p)
	
	def __init__(self, _url=None):
		if not _url:
			raise Exception("Not implemented yet create new urls")
		self._url=_url
		self.cbs=[]

	def add(self, regex, callback):
		def cb(ignore, request, response):
			self.cbs # to keep cb existing, need a link to the url, although does nothing
			try:
				return callback(Request(request), Response(response))
			except:
				import traceback
				traceback.print_exc()
				return OCS_INTERNAL_ERROR
		
		_cb=Url.CBFUNC(cb)
		self.cbs.append( (cb,_cb, callback) )
		
		_libonion.onion_url_add(self._url, regex, _cb)
