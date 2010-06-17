from ctypes import *
from pydfhack import libdfhack, ViewScreen

libdfhack.Gui_ReadViewScreen.argtypes = [ c_void_p, c_void_p ]

class Gui(object):
    def __init__(self, ptr):
        self._gui_ptr = ptr

    def start(self):
        return libdfhack.Gui_Start(self._gui_ptr)

    def finish(self):
        return libdfhack.Gui_Finish(self._gui_ptr)

    def read_pause_state(self):
        return libdfhack.Gui_ReadPauseState(self._pos_ptr) > 0

    def read_view_screen(self):
        s = ViewScreen()

        if libdfhack.Gui_ReadViewScreen(self._gui_ptr, byref(s)) > 0:
            return s
        else:
            return None
