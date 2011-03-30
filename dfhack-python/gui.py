from ctypes import c_void_p, c_int, c_uint, byref
from dftypes import libdfhack, ViewScreen, Hotkey
from util import check_pointer_cache

libdfhack.Gui_getViewCoords.argtypes = [ c_void_p, POINTER(c_int), POINTER(c_int), POINTER(c_int) ]
libdfhack.Gui_setViewCoords.argtypes = [ c_void_p, c_int, c_int, c_int ]

libdfhack.Gui_getCursorCoords.argtypes = [ c_void_p, POINTER(c_int), POINTER(c_int), POINTER(c_int) ]
libdfhack.Gui_setCursorCoords.argtypes = [ c_void_p, c_int, c_int, c_int ]

libdfhack.Gui_getWindowSize.argtypes = [ c_void_p, POINTER(c_int), POINTER(c_int) ]

libdfhack.Gui_ReadViewScreen.argtypes = [ c_void_p, c_void_p ]

libdfhack.Gui_ReadHotkeys.restype = c_void_p

libdfhack.Gui_getScreenTiles.argtypes = [ c_void_p, c_int, c_int ]
libdfhack.Gui_getScreenTiles.restype = c_void_p

class Gui(object):
    def __init__(self, ptr):
        self._gui_ptr = ptr

    def start(self):
        return libdfhack.Gui_Start(self._gui_ptr) > 0

    def finish(self):
        return libdfhack.Gui_Finish(self._gui_ptr) > 0

    def read_view_screen(self):
        s = ViewScreen()

        if libdfhack.Gui_ReadViewScreen(self._gui_ptr, byref(s)) > 0:
            return s
        else:
            return None

    def get_view_coords(self):
        x, y, z = (0, 0, 0)
        
        if libdfhack.Gui_getViewCoords(self._gui_ptr, byref(x), byref(y), byref(z)) > 0:
            return (x, y, z)
        else:
            return (-1, -1, -1)

    def set_view_coords(self, x, y, z):
        libdfhack.Gui_setViewCoords(self._gui_ptr, x, y, z)
    
    def get_cursor_coords(self):
        x, y, z = (0, 0, 0)
        
        if libdfhack.Gui_getCursorCoords(self._gui_ptr, byref(x), byref(y), byref(z)) > 0:
            return (x, y, z)
        else:
            return (-1, -1, -1)

    def set_cursor_coords(self, x, y, z):
        libdfhack.Gui_setCursorCoords(self._gui_ptr, x, y, z)
    
    def read_hotkeys(self):
        return check_pointer_cache(libdfhack.Gui_ReadHotkeys(self._gui_ptr))
    
    def get_screen_tiles(self, width, height):
        return check_pointer_cache(libdfhack.Gui_getScreenTiles(self._gui_ptr, width, height))

    @property
    def window_size(self):
        width, height = (0, 0)
        
        if libdfhack.Gui_getWindowSize(self._gui_ptr, byref(width), byref(height)) > 0:
            return (width, height)
        else:
            return (-1, -1)
