from ctypes import *

int_ptr = POINTER(c_int)
uint_ptr = POINTER(c_uint)

short_ptr = POINTER(c_short)
ushort_ptr = POINTER(c_ushort)

byte_ptr = POINTER(c_byte)
ubyte_ptr = POINTER(c_ubyte)

pointer_dict = {}

def _uintify(x, y, z):
    return (c_uint(x), c_uint(y), c_uint(z))

def _allocate_array(ptr, t_type, count):
    arr = (t_type * count)()
    
    p = cast(arr, POINTER(t_type))
    
    ptr[0] = p
    
    pointer_dict[addressof(arr)] = (ptr, arr, p)

    return 1

def _alloc_int_buffer(ptr, count):
    return _allocate_array(ptr, c_int, count)

_int_functype = CFUNCTYPE(c_int, POINTER(POINTER(c_int)), c_uint)
alloc_int_buffer = _int_functype(_alloc_int_buffer)

def _alloc_uint_buffer(ptr, count):
    return _allocate_array(ptr, c_uint, count)

_uint_functype = CFUNCTYPE(c_int, POINTER(POINTER(c_uint)), c_uint)
alloc_uint_buffer = _uint_functype(_alloc_uint_buffer)

def _alloc_short_buffer(ptr, count):
    return _allocate_array(ptr, c_short, count)

_short_functype = CFUNCTYPE(c_int, POINTER(POINTER(c_short)), c_uint)
alloc_short_buffer = _short_functype(_alloc_short_buffer)

def _alloc_ushort_buffer(ptr, count):
    return _allocate_array(ptr, c_ushort, count)

_ushort_functype = CFUNCTYPE(c_int, POINTER(POINTER(c_ushort)), c_uint)
alloc_ushort_buffer = _ushort_functype(_alloc_ushort_buffer)

def _alloc_byte_buffer(ptr, count):
    return _allocate_array(ptr, c_byte, count)

_byte_functype = CFUNCTYPE(c_int, POINTER(POINTER(c_byte)), c_uint)
alloc_byte_buffer = _byte_functype(_alloc_byte_buffer)

def _alloc_ubyte_buffer(ptr, count):
    return _allocate_array(ptr, c_ubyte, count)

_ubyte_functype = CFUNCTYPE(c_int, POINTER(POINTER(c_ubyte)), c_uint)
alloc_ubyte_buffer = _ubyte_functype(_alloc_ubyte_buffer)

def _alloc_char_buffer(ptr, count):
    c = create_string_buffer(count)
    
    p = cast(c, POINTER(c_char))

    ptr[0] = p
    
    pointer_dict[id(ptr[0])] = (ptr, c, p)

    return 1

_char_functype = CFUNCTYPE(c_int, POINTER(POINTER(c_char)), c_uint)
alloc_char_buffer = _char_functype(_alloc_char_buffer)
