from ctypes import *

int_ptr = POINTER(c_int)
uint_ptr = POINTER(c_uint)

libdfhack = cdll.libdfhack
libdfhack.API_Alloc.restype = c_void_p
libdfhack.API_Free.argtypes = [ c_void_p ]

class API(object):
    def __init__(self, memory_path):
        self._api_ptr = libdfhack.API_Alloc(create_string_buffer(memory_path))

    def __del__(self):
        libdfhack.API_Free(self._api_ptr)

    def Attach(self):
        return libdfhack.API_Attach(self._api_ptr) > 0

    def Detach(self):
        return libdfhack.API_Detach(self._api_ptr) > 0

    def Suspend(self):
        return libdfhack.API_Suspend(self._api_ptr) > 0

    def Resume(self):
        return libdfhack.API_Resume(self._api_ptr) > 0

    def Force_Resume(self):
        return libdfhack.API_ForceResume(self._api_ptr) > 0

    def Async_Suspend(self):
        return libdfhack.API_AsyncSuspend(self._api_ptr) > 0

    @property
    def is_attached(self):
        return libdfhack.API_isAttached(self._api_ptr) > 0

    @property
    def is_suspended(self):
        return libdfhack.API_isSuspended(self._api_ptr) > 0
