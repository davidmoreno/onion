from . import _libonion, OCS_INTERNAL_ERROR
from .request import Request
from .response import Response
from ctypes import CFUNCTYPE, c_int, c_void_p, c_char_p

class Url(object):
	CBFUNC=CFUNCTYPE(c_int, c_void_p, c_void_p, c_void_p)
	
	def __init__(self, _ptr=None):
		#print _ptr, type(_ptr)
		assert isinstance(_ptr, c_void_p)
		self._url=_ptr
		self.cbs=[]

	def add(self, regex, callback):
		def cb(ignore, request, response):
			self.cbs # to keep cb existing, need a link to the url, although does nothing
			try:
				return callback(Request(c_void_p(request)), Response(c_void_p(response)))
			except:
				import traceback
				traceback.print_exc()
				return OCS_INTERNAL_ERROR
		
		_cb=Url.CBFUNC(cb)
		self.cbs.append( (cb,_cb, callback) )
		
		_libonion.onion_url_add(self._url, regex, _cb)

	def add_static(self, regex, text, code=200):
		ss=str.encode(text)
		cs=c_char_p(ss)
		_libonion.onion_url_add_static(self._url, regex, cs, code)
