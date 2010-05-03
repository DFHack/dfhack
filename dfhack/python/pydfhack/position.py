# -*- coding: utf-8 -*-
"""
Python class for DF_Hack::Position
"""
from ._pydfhack import _PositionManager
from .blocks import Point, Block
class Position(_PositionManager):
    api = None
    def __init__(self, api, *args, **kwds):
        _PositionManager.__init__(self, args, kwds)
        self.api = api

    def prepare(self):
        """
        Enforce Suspend/Start
        """
        return self.api.prepare()

    def get_cursor(self):
        self.prepare()
        coords = self.cursor_coords
        return Point(*coords)

