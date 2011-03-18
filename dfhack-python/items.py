from ctypes import *
from dftypes import *

libdfhack.Items_getItemDescription.argtypes = [ c_void_p, c_uint, c_void_ptr, _arr_create_func ]
libdfhack.Items_getItemDescription.restype = c_char_p
libdfhack.Items_getItemClass.argtypes = [ c_void_p, c_int, _arr_create_func ]
libdfhack.Item_getItemClass.restype = c_char_p

class Items(object):
    def __init__(self, ptr):
        self._i_ptr = ptr

    def get_item_description(self, itemptr, materials):
        def get_callback(count):
            item_string = create_string_buffer(count)

            return byref(item_string)

        item_string = None
        callback = _arr_create_func(get_callback)

        return libdfhack.Items_getItemDescription(self._i_ptr, itemptr, materials, callback)

    def get_item_class(self, index):
        def get_callback(count):
            item_string = create_string_buffer(count)

            return byref(item_string)

        item_string = None
        callback = _arr_create_func(get_callback)

        return libdfhack.Items_getItemClass(self._i_ptr, index, callback)
