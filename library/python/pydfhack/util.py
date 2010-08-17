from ctypes import *

uint_ptr = POINTER(c_uint)
int_ptr = POINTER(c_int)

def _uintify(x, y, z):
    return (c_uint(x), c_uint(y), c_uint(z))

def _allocate_array(t_type, count):
    arr_type = t_type * count

    arr = arr_type()

    ptr = c_void_p()
    ptr = addressof(arr)

    return (arr, ptr)

def _alloc_int_buffer(ptr, count):
    a = _allocate_array(c_int, count)

    ptr = addressof(a[0])

    return 1

_int_functype = CFUNCTYPE(c_int, POINTER(c_int), c_uint)
alloc_int_buffer = _int_functype(_alloc_int_buffer)

def _alloc_uint_buffer(ptr, count):
    a = _allocate_array(c_uint, count)

    ptr = addressof(a[0])

    return 1

_uint_functype = CFUNCTYPE(c_int, POINTER(c_uint), c_uint)
alloc_uint_buffer = _uint_functype(_alloc_uint_buffer)

def _alloc_short_buffer(ptr, count):
    a = _allocate_array(c_short, count)

    ptr = addressof(a[0])

    return 1

_short_functype = CFUNCTYPE(c_int, POINTER(c_short), c_uint)
alloc_short_buffer = _short_functype(_alloc_short_buffer)

def _alloc_ushort_buffer(ptr, count):
    a = _allocate_array(c_ushort, count)

    ptr = addressof(a[0])

    return 1

_ushort_functype = CFUNCTYPE(c_int, POINTER(c_ushort), c_uint)
alloc_ushort_buffer = _ushort_functype(_alloc_ushort_buffer)

def _alloc_byte_buffer(ptr, count):
    a = _allocate_array(c_byte, count)

    ptr = addressof(a[0])

    return 1

_byte_functype = CFUNCTYPE(c_int, POINTER(c_byte), c_uint)
alloc_byte_buffer = _byte_functype(_alloc_byte_buffer)

def _alloc_ubyte_buffer(ptr, count):
    a = _allocate_array(c_ubyte, count)

    ptr = addressof(a[0])

    return 1

_ubyte_functype = CFUNCTYPE(c_int, POINTER(c_ubyte), c_uint)
alloc_ubyte_buffer = _ubyte_functype(_alloc_ubyte_buffer)

def _alloc_char_buffer(ptr, count):
    c = create_string_buffer(count)

    if ptr is None:
        ptr = c_void_p

    ptr = addressof(c)

    return 1

_char_functype = CFUNCTYPE(c_int, POINTER(c_char), c_uint)
alloc_char_buffer = _char_functype(_alloc_char_buffer)
