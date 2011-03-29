from ctypes import c_uint, byref
from dftypes import libdfhack, Construction

class Constructions(object):
    def __init__(self, ptr):
        self._c_ptr = ptr

    def start(self):
        num = c_uint()

        if libdfhack.Constructions_Start(self._c_ptr, byref(num)) > 0:
            return int(num.value)
        else:
            return -1

    def finish(self):
        return libdfhack.Constructions_Finish(self._c_ptr) > 0

    def read(self, index):
        c = Construction()

        if libdfhack.Constructions_Read(self._c_ptr, c_uint(index), byref(c)) > 0:
            return c
        else:
            return None
