from ctypes import *
from dftypes import libdfhack
from util import check_pointer_cache

libdfhack.Process_readQuad.argtypes = [ c_void_p, c_uint, POINTER(c_ulong) ]
libdfhack.Process_writeQuad.argtypes = [ c_void_p, c_uint, c_ulong ]

libdfhack.Process_readDWord.argtypes = [ c_void_p, c_uint, POINTER(c_uint) ]
libdfhack.Process_writeDWord.argtypes = [ c_void_p, c_uint, c_uint ]

libdfhack.Process_readWord.argtypes = [ c_void_p, c_uint, POINTER(c_ushort) ]
libdfhack.Process_writeWord.argtypes = [ c_void_p, c_uint, c_ushort ]

libdfhack.Process_readByte.argtypes = [ c_void_p, c_uint, POINTER(c_ubyte) ]
libdfhack.Process_writeByte.argtypes = [ c_void_p, c_uint, c_ubyte ]

libdfhack.Process_readFloat.argtypes = [ c_void_p, c_uint, POINTER(c_float) ]

libdfhack.Process_read.argtypes = [ c_void_p, c_uint, c_uint ]
libdfhack.Process_read.restype = c_void_p

libdfhack.Process_write.argtypes = [ c_void_p, c_uint, c_uint, POINTER(c_ubyte) ]

libdfhack.Process_getThreadIDs.restype = c_void_p

class Process(object):
    def __init__(self, ptr):
        self._p_ptr = ptr
    
    def attach(self):
        return libdfhack.Process_attach(self._p_ptr) > 0
    
    def detach(self):
        return libdfhack.Process_detach(self._p_ptr) > 0
    
    def suspend(self):
        return libdfhack.Process_suspend(self._p_ptr) > 0
    
    def async_suspend(self):
        return libdfhack.Process_asyncSuspend(self._p_ptr) > 0
    
    def resume(self):
        return libdfhack.Process_resume(self._p_ptr) > 0
    
    def force_resume(self):
        return libdfhack.Process_forceresume(self._p_ptr) > 0
    
    def read_quad(self, address):
        q = c_ulong(0)
        
        if libdfhack.Process_readQuad(self._p_ptr, address, byref(q)) > 0:
            return long(q.value)
        else:
            return -1
    
    def write_quad(self, address, value):
        return libdfhack.Process_writeQuad(self._p_ptr, address, value) > 0
    
    def read_dword(self, address):
        d = c_uint(0)
        
        if libdfhack.Process_readDWord(self._p_ptr, address, byref(d)) > 0:
            return int(d.value)
        else:
            return -1
    
    def write_dword(self, address, value):
        return libdfhack.Process_writeDWord(self._p_ptr, address, value) > 0
    
    def read_word(self, address):
        s = c_ushort(0)
        
        if libdfhack.Process_readWord(self._p_ptr, address, byref(s)) > 0:
            return int(s.value)
        else:
            return -1
    
    def write_word(self, address, value):
        return libdfhack.Process_writeWord(self._p_ptr, address, value) > 0
    
    def read_byte(self, address):
        b = c_ubyte(0)
        
        if libdfhack.Process_readByte(self._p_ptr, address, byref(b)) > 0:
            return int(b.value)
        else:
            return -1
    
    def write_byte(self, address, value):
        return libdfhack.Process_writeByte(self._p_ptr, address, value) > 0
    
    def read_float(self, address):
        f = c_float(0)
        
        if libdfhack.Process_readFloat(self._p_ptr, address, byref(f)) > 0:
            return float(f.value)
        else:
            return -1
    
    def read(self, address, length):
        return check_pointer_cache(libdfhack.Process_read(self._p_ptr, address, length))
    
    def write(self, address, length, buffer):
        libdfhack.Process_write(self._p_ptr, address, length, byref(buffer))
    
    def get_thread_ids(self):
        return check_pointer_cache(libdfhack.Process_getThreadIDs(self._p_ptr))
    
    def set_and_wait(self, state):
        return libdfhack.Process_SetAndWait(self._p_ptr, state) > 0
    
    @property
    def is_suspended(self):
        return libdfhack.Process_isSuspended(self._p_ptr) > 0
    
    @property
    def is_attached(self):
        return libdfhack.Process_isAttached(self._p_ptr) > 0
    
    @property
    def is_identified(self):
        return libdfhack.Process_isIdentified(self._p_ptr) > 0
    
    @property
    def is_snapshot(self):
        return libdfhack.Process_isSnapshot(self._p_ptr) > 0
    
    @property
    def pid(self):
        p = c_int(0)
        
        if libdfhack.Process_getPID(self._p_ptr, byref(p)) > 0:
            return int(p.value)
        else:
            return -1