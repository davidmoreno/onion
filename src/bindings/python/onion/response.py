from . import _libonion
from ctypes import c_char_p, c_void_p

class Response(object):
	def __init__(self, _ptr):
		assert isinstance(_ptr, c_void_p)
		self._request=_ptr
		self._write0=_libonion.onion_response_write0
		self._write_safe=_libonion.onion_response_write_html_safe

	def write(self, s):
		n=self._write0(self._request, s)
		return n

	def write_html_safe(self, s):
		return self._write_safe(self._request, s)
