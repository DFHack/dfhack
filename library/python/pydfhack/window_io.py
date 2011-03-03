from ctypes import *

libdfhack.WindowIO_TypeStr.argtypes = [ c_void_p, c_char_p, c_uint, c_byte ]
libdfhack.WindowIO_TypeSpecial.argtypes = [ c_void_p, c_uint, c_uint, c_uint, c_uint ]

class WindowIO(object):
    def __init__(self, ptr):
        self._window_io_ptr = ptr
    
    def type_str(self, s, delay = 0, use_shift = False):
        c_shift = c_byte(0)
        c_delay = c_int(delay)
        c_s = c_char_p(s)
        
        if use_shift is True:
            c_shift = c_byte(1)
        
        return libdfhack.WindowIO_TypeStr(self._window_io_ptr, c_s, c_delay, c_shift) > 0
    
    def type_special(self, command, count = 1, delay = 0):
        c_command = c_uint(command)
        c_count = c_uint(count)
        c_delay = c_uint(delay)
        
        return libdfhack.WindwIO_TypeSpecial(self._window_io_ptr, c_command, c_count, c_delay) > 0