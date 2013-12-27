from . import _libonion
from ctypes import c_void_p, c_char_p
from .dict import Dict


class Request(object):
	def __init__(self, _ptr):
		assert isinstance(_ptr, c_void_p)
		self._request=_ptr

	def query(self):
		return Dict( c_void_p( _libonion.onion_request_get_query_dict(self._request) ) )
	
	def path(self):
		return _libonion.onion_request_get_path(self._request)
	
	def fullpath(self):
		return _libonion.onion_request_get_fullpath(self._request)

	def header(self):
		return Dict( c_void_p( _libonion.onion_request_get_header_dict( self._request) ) )

_libonion.onion_request_get_query_dict.restype=c_void_p
_libonion.onion_request_get_path.restype=c_char_p
_libonion.onion_request_get_fullpath.restype=c_char_p
_libonion.onion_request_get_header_dict.restype=c_void_p

