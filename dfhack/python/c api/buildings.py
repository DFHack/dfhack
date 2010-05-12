from ctypes import *
from pydftypes import *
import util

libdfhack.Buildings_GetCustomWorkshopType.argtypes = [ c_void_p, POINTER(CustomWorkshop) ]

def Buildings(object):
    def __init__(self, ptr):
        self._b_ptr = ptr

    def start(self):
        num = c_uint()

        if libdfhack.Buildings_Start(self._b_ptr, byref(num)) > 0:
            return int(num.value)
        else:
            return -1

    def finish(self):
        return libdfhack.Buildings_Finish(self._b_ptr) > 0

    def read(self, index):
        b = Building()

        if libdfhack.Buildings_Read(self._b_ptr, c_uint(index), byref(b)) > 0:
            return b
        else:
            return None

    def read_custom_workshop_types(self):
        def read_callback(count):
            allocated = util._allocate_array(CustomWorkshop, count)
            
            workshop_types = allocated[0]

            return allocated[1]

        workshop_types = None
        callback = _arr_create_func(read_callback)

        if libdfhack.Buildings_ReadCustomWorkshopTypes(self._b_ptr, callback) > 0:
            return workshop_types
        else:
            return None

    def get_custom_workshop_type(self, custom_workshop):
        return libdfhack.Buildings_GetCustomWorkshopType(self._b_ptr, byref(custom_workshop))
