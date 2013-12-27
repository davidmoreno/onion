import ctypes


_libonion=ctypes.cdll.LoadLibrary("libonion.so")

class Onion(object):
	def __init__(self, flags):
		self._onion=_libonion.onion_new(flags)
		assert self._onion, "Onion object not correctly created"

	def listen(self):
		_libonion.onion_listen(self._onion)
		
	def listen_stop(self):
		_libonion.onion_listen_stop(self._onion)
		
	def root_url(self):
		from .url import Url
		return Url(ctypes.c_void_p(_libonion.onion_root_url(self._onion)))
		
	def __destroy__(self):
		_libonion.onion_free(self._onion)

_libonion.onion_new.restype=ctypes.c_void_p
_libonion.onion_root_url.restype=ctypes.c_void_p

# some flags for onion
O_ONE=1
O_ONE_LOOP=3
O_THREADED=4
O_DETACH_LISTEN=8
O_SYSTEMD=0x010
O_POLL=0x020
O_POOL=0x024
O_SSL_AVAILABLE=0x0100
O_SSL_ENABLED=0x0200

O_THREADS_AVALIABLE=0x0400
O_THREADS_ENABLED=0x0800

O_DETACHED=0x01000

# response types
OCS_NOT_PROCESSED=0
OCS_NEED_MORE_DATA=1
OCS_PROCESSED=2
OCS_CLOSE_CONNECTION=-2
OCS_KEEP_ALIVE=3
OCS_WEBSOCKET=4
OCS_INTERNAL_ERROR=-500
OCS_NOT_IMPLEMENTED=-501
OCS_FORBIDDEN=-502
