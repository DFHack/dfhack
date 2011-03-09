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

def _allocate_array(t_type, count):
    arr_type = t_type * count

    arr = arr_type()

    return arr

def _alloc_int_buffer(ptr, count):
    a = _allocate_array(c_int, count)
    
    p = cast(a, int_ptr)

    ptr[0] = p
    
    pointer_dict[id(ptr[0])] = (ptr, a, p)

    return 1

_int_functype = CFUNCTYPE(c_int, POINTER(POINTER(c_int)), c_uint)
alloc_int_buffer = _int_functype(_alloc_int_buffer)

def _alloc_uint_buffer(ptr, count):
    a = _allocate_array(c_uint, count)

    ptr[0] = cast(a, uint_ptr)
    
    pointer_dict[id(ptr[0])] = (ptr, a, p)

    return 1

_uint_functype = CFUNCTYPE(c_int, POINTER(POINTER(c_uint)), c_uint)
alloc_uint_buffer = _uint_functype(_alloc_uint_buffer)

def _alloc_short_buffer(ptr, count):
    a = _allocate_array(c_short, count)

    ptr[0] = cast(a, short_ptr)
    
    pointer_dict[id(ptr[0])] = (ptr, a, p)

    return 1

_short_functype = CFUNCTYPE(c_int, POINTER(POINTER(c_short)), c_uint)
alloc_short_buffer = _short_functype(_alloc_short_buffer)

def _alloc_ushort_buffer(ptr, count):
    a = _allocate_array(c_ushort, count)

    ptr[0] = cast(a, ushort_ptr)
    
    pointer_dict[id(ptr[0])] = (ptr, a, p)

    return 1

_ushort_functype = CFUNCTYPE(c_int, POINTER(POINTER(c_ushort)), c_uint)
alloc_ushort_buffer = _ushort_functype(_alloc_ushort_buffer)

def _alloc_byte_buffer(ptr, count):
    a = _allocate_array(c_byte, count)

    ptr[0] = cast(a, byte_ptr)
    
    pointer_dict[id(ptr[0])] = (ptr, a, p)

    return 1

_byte_functype = CFUNCTYPE(c_int, POINTER(POINTER(c_byte)), c_uint)
alloc_byte_buffer = _byte_functype(_alloc_byte_buffer)

def _alloc_ubyte_buffer(ptr, count):
    a = _allocate_array(c_ubyte, count)

    ptr[0] = cast(a, ubyte_ptr)
    
    pointer_dict[id(ptr[0])] = (ptr, a, p)

    return 1

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
