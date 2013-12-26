from . import _libonion
from ctypes import c_char_p, c_void_p

class Response(object):
	def __init__(self, ptr):
		self._request=c_void_p(ptr)
		self._write0=_libonion.onion_response_write0

	def write(self, s):
		ss=str(s)
		cs=c_char_p(ss)
		n=self._write0(self._request, cs)
		return n
