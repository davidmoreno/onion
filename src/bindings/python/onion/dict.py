from . import _libonion
from ctypes import CFUNCTYPE, c_void_p, c_char_p, c_int

OD_FREE_KEY=2
OD_FREE_VALUE=4
OD_FREE_ALL=6
OD_DUP_KEY=0x12
OD_DUP_VALUE=0x24
OD_DUP_ALL=0x36
OD_REPLACE=0x040
OD_STRING=0
OD_DICT=0x0100
OD_TYPE_MASK=0x0FF00
OD_ICASE=0x01

class Dict(object):
	def __init__(self, _ptr, free=False):
		#print _ptr, type(_ptr)
		assert isinstance(_ptr, c_void_p)
		self._free=free
		self._dict=_ptr
		#print 'new dict', self._dict
		
	def __destroy__(self):
		if self._free:
			_libonion.onion_dict_free(self._dict)
		
	def copy(self):
		#print 'to dict'
		d={}
		def add_to_dict(ignore, key, value, flags):
			d[key.decode('ascii')]=value.decode('ascii')
		_libonion.onion_dict_preorder(self._dict, CFUNCTYPE(None, c_void_p, c_char_p, c_char_p, c_int)(add_to_dict))
		#print d
		return d

	def __getitem__(self, key):
		#import pdb; pdb.set_trace()
		#print 'get item',self._dict, key
		v=_libonion.onion_dict_get(self._dict, key)
		if v:
			return v
		return Dict( _libonion.onion_dict_get_dict(self._dict, key) )

	def __setitem__(self, key, val):
		if isinstance(val, Dict):
			_libonion.onion_dict_add(self._dict, key, val.ptr, OD_DUP_ALL|OD_DICT)
		else:
			_libonion.onion_dict_add(self._dict, key, val.ptr, OD_DUP_ALL)

_libonion.onion_dict_get_dict.restype=Dict
_libonion.onion_dict_get.restype=c_char_p
