from ctypes import *

def _uintify(x, y, z):
    return (c_uint(x), c_uint(y), c_uint(z))

def _allocate_array(t_type, count):
    arr_type = t_type * count

    arr = arr_type()

    ptr = c_void_p()
    ptr = addressof(arr)

    return (arr, ptr)

def _alloc_int_buffer(count):
    a = _allocate_array(c_int, count)

    return a[1]

alloc_int_buffer = CFUNCTYPE(POINTER(c_int), c_uint)(_alloc_int_buffer)

def _alloc_uint_buffer(count):
    a = _allocate_array(c_uint, count)

    return a[1]

alloc_uint_buffer = CFUNCTYPE(POINTER(c_uint), c_uint)(_alloc_uint_buffer)

def _alloc_short_buffer(count):
    a = _allocate_array(c_short, count)

    return a[1]

alloc_short_buffer = CFUNCTYPE(POINTER(c_short), c_uint)(_alloc_short_buffer)

def _alloc_ushort_buffer(count):
    a = _allocate_array(c_ushort, count)

    return a[1]

alloc_ushort_buffer = CFUNCTYPE(POINTER(c_ushort), c_uint)(_alloc_ushort_buffer)

def _alloc_byte_buffer(count):
    a = _allocate_array(c_byte, count)

    return a[1]

alloc_byte_buffer = CFUNCTYPE(POINTER(c_byte), c_uint)(_alloc_byte_buffer)

def _alloc_ubyte_buffer(count):
    a = _allocate_array(c_ubyte, count)

    return a[1]

alloc_ubyte_buffer = CFUNCTYPE(POINTER(c_ubyte), c_uint)(_alloc_ubyte_buffer)

def _alloc_char_buffer(count):
    c = create_string_buffer(count)

    ptr = c_void_p()
    ptr = addressof(c)

    return ptr

alloc_char_buffer = CFUNCTYPE(POINTER(c_char), c_uint)(_alloc_char_buffer)
