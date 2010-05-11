from ctypes import *

def _uintify(x, y, z):
    return (c_uint(x), c_uint(y), c_uint(z))

def _allocate_array(t_type, count):
    arr_type = t_type * count

    arr = arr_type()

    ptr = c_void_p()
    ptr = addressof(arr)

    return (arr, ptr)
