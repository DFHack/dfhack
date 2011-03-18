from ctypes import *
from pydfhack import libdfhack, ViewScreen

libdfhack.Gui_ReadHotkeys.restype = c_void_p
libdfhack.Gui_ReadViewScreen.argtypes = [ c_void_p, c_void_p ]

class Gui(object):
    def __init__(self, ptr):
        self._gui_ptr = ptr

        self._vx, self._vy, self._vz = c_int(), c_int(), c_int()
        self._cx, self._cy, self._cz = c_int(), c_int(), c_int()
        self._ww, self._wh = c_int(), c_int()

    def start(self):
        return libdfhack.Gui_Start(self._gui_ptr)

    def finish(self):
        return libdfhack.Gui_Finish(self._gui_ptr)

    def read_view_screen(self):
        s = ViewScreen()

        if libdfhack.Gui_ReadViewScreen(self._gui_ptr, byref(s)) > 0:
            return s
        else:
            return None

    def get_view_coords(self):
        if libdfhack.Gui_getViewCoords(self._gui_ptr, byref(self._vx), byref(self._vy), byref(self._vz)) > 0:
            return (self._vx.value, self._vy.value, self._vz.value)
        else:
            return (-1, -1, -1)

    def set_view_coords(self, v_coords):
        self._vx.value, self._vy.value, self._vz.value = v_coords

        libdfhack.Gui_setViewCoords(self._gui_ptr, self._vx, self._vy, self._vz)

    view_coords = property(get_view_coords, set_view_coords)
    
    def get_cursor_coords(self):
        if libdfhack.Gui_getCursorCoords(self._gui_ptr, byref(self._cx), byref(self._cy), byref(self._cz)) > 0:
            return (self._cx.value, self._cy.value, self._cz.value)
        else:
            return (-1, -1, -1)

    def set_cursor_coords(self, c_coords):
        self._cx.value, self._cy.value, self_cz.value = c_coords

        libdfhack.Gui_setCursorCoords(self._gui_ptr, self._cx, self._cy, self._cz)

    cursor_coords = property(get_cursor_coords, set_cursor_coords)
    
    def read_hotkeys(self):
        return check_pointer_cache(libdfhack.Gui_ReadHotkeys(self._gui_ptr))

    @property
    def window_size(self):
        if libdfhack.Gui_getWindowSize(self._gui_ptr, byref(self._ww), byref(self._wh)) > 0:
            return (self._ww.value, self._wh.value)
        else:
            return (-1, -1)
