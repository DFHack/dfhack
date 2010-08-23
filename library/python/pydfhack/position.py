from ctypes import *
from dftypes import libdfhack

class Position(object):
    def __init__(self, ptr):
        self._pos_ptr = ptr

        self._vx, self._vy, self._vz = c_int(), c_int(), c_int()
        self._cx, self._cy, self._cz = c_int(), c_int(), c_int()
        self._ww, self._wh = c_int(), c_int()

    def get_view_coords(self):
        if libdfhack.Position_getViewCoords(self._pos_ptr, byref(self._vx), byref(self._vy), byref(self._vz)) > 0:
            return (self._vx.value, self._vy.value, self._vz.value)
        else:
            return (-1, -1, -1)

    def set_view_coords(self, v_coords):
        self._vx.value, self._vy.value, self._vz.value = v_coords

        libdfhack.Position_setViewCoords(self._pos_ptr, self._vx, self._vy, self._vz)

    view_coords = property(get_view_coords, set_view_coords)
    
    def get_cursor_coords(self):
        if libdfhack.Position_getCursorCoords(self._pos_ptr, byref(self._cx), byref(self._cy), byref(self._cz)) > 0:
            return (self._cx.value, self._cy.value, self._cz.value)
        else:
            return (-1, -1, -1)

    def set_cursor_coords(self, c_coords):
        self._cx.value, self._cy.value, self_cz.value = c_coords

        libdfhack.Position_setCursorCoords(self._pos_ptr, self._cx, self._cy, self._cz)

    cursor_coords = property(get_cursor_coords, set_cursor_coords)

    @property
    def window_size(self):
        if libdfhack.Position_getWindowSize(self._pos_ptr, byref(self._ww), byref(self._wh)) > 0:
            return (self._ww.value, self._wh.value)
        else:
            return (-1, -1)
