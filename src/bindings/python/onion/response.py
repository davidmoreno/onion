from . import _libonion
from ctypes import c_char_p, c_void_p

class Response(object):
	_write=_libonion.onion_response_write
	_write0=_libonion.onion_response_write0
	_write_safe=_libonion.onion_response_write_html_safe
	_set_header=_libonion.onion_response_set_header
	_set_code=_libonion.onion_response_set_code
	_set_length=_libonion.onion_response_set_length
	
	def __init__(self, _ptr):
		assert isinstance(_ptr, c_void_p)
		self._request=_ptr

	def write(self, s, n=None):
		if not n:
			n=len(s)
		n=Response._write(self._request, s, n)
		return n

	def write_html_safe(self, s):
		return Response._write_safe(self._request, s)

	def set_header(self, key, value):
		Response._set_header(self._request, key, value)
		
	def set_code(self, code):
		Response._set_code(self._request, code)		
		
	def set_length(self, length):
		Response._set_length(self._request, length)

HTTP_OK=200
HTTP_CREATED=201
HTTP_PARTIAL_CONTENT=206
HTTP_MULTI_STATUS=207


HTTP_MOVED=301
HTTP_REDIRECT=302
HTTP_SEE_OTHER=303
HTTP_NOT_MODIFIED=304
HTTP_TEMPORARY_REDIRECT=307

HTTP_BAD_REQUEST=400
HTTP_UNAUTHORIZED=401
HTTP_FORBIDDEN=403
HTTP_NOT_FOUND=404
HTTP_METHOD_NOT_ALLOWED=405

HTTP_INTERNAL_ERROR=500
HTTP_NOT_IMPLEMENTED=501
HTTP_BAD_GATEWAY=502
HTTP_SERVICE_UNAVALIABLE=503
