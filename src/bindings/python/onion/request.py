from . import _libonion

class Request(object):
	def __init__(self, ptr):
		self._request=ptr
