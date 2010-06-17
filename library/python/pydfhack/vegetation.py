from ctypes import *
from pydftypes import libdfhack, Tree

class Vegetation(object):
    def __init__(self, ptr):
        self._v_ptr = ptr

    def start(self):
        n = c_uint(0)
        
        if libdfhack.Vegetation_Start(self._v_ptr, byref(n)) > 0:
            return int(n.value)
        else:
            return -1

    def finish(self):
        return libdfhack.Vegetation_Finish(self._v_ptr) > 0

    def read(self, index):
        t = Tree()

        if libdfhack.Vegetation_Read(self._v_ptr, c_uint(index), byref(t)) > 0:
            return t
        else:
            return None
