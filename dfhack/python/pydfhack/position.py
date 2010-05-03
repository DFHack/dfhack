# -*- coding: utf-8 -*-
"""
Python class for DF_Hack::Position
"""
from ._pydfhack import _PositionManager
from .blocks import Point, Block
from .mixins import NoStart
from .decorators import suspend

class Position(NoStart, _PositionManager):
    api = None
    cls = _PositionManager
    def __init__(self, api, *args, **kwds):
        self.cls.__init__(self, args, kwds)
        self.api = api

    @suspend
    def get_cursor(self):
        coords = self.cursor_coords
        return Point(*coords)

    @suspend
    def get_window_size(self):
        wsize = self.window_size
        return wsize

    @suspend
    def get_view_coords(self):
        coords = self.view_coords
        return Point(*coords)
