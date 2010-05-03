# -*- coding: utf-8 -*-
"""
Python class for DF_Hack::Maps
"""
from ._pydfhack import _MapManager
from .mixins import NeedsStart
from .decorators import suspend
from .blocks import Block, Point

class Map(NeedsStart, _MapManager):
    api = None
    cls = _MapManager
    def __init__(self, api, *args, **kwds):
        self.cls.__init__(self, args, kwds)
        self.api = api

    @suspend
    def get_size(self):
        return self.size

    @suspend
    def Is_Valid_Block(self, x=0, y=0, z=0, point=None):
        """
        Checks if coords are valid
        """
        if point:
            point = point.get_block()
            x, y, z = point.xyz        
        return self.cls.Is_Valid_Block(self, x, y, z)

    @suspend
    def Read_Tile_Types(self, x=0, y=0, z=0, point=None):
        """
        Returns 16x16 block in form of list of lists.
        Should be called either by point or x,y,z coords. Coords are in block form.
        """
        if point:
            point = point.get_block()
            x, y, z = point.xyz
        return self.cls.Read_Tile_Types(self, x, y, z)

    @suspend
    def Write_Tile_Types(self, x=0, y=0, z=0, tiles=None, point=None):
        if point:
            point = point.get_block()
            x, y, z = point.xyz        
        return self.cls.Write_Tile_Types(self, x, y, z, tiles)

    @suspend
    def Read_Designations(self, x=0, y=0, z=0, point=None):
        """
        Returns 16x16 block in form of list of lists.
        """
        if point:
            point = point.get_block()
            x, y, z = point.xyz
        return self.cls.Read_Designations(self, x, y, z)

    @suspend
    def Write_Designations(self, x=0, y=0, z=0, tiles=None, point=None):
        if point:
            point = point.get_block()
            x, y, z = point.xyz           
        return self.cls.Write_Designations(self, x, y, z, tiles)

    @suspend
    def Write_Occupancy(self, *args, **kw):
        return self.cls.Write_Occupancy(self, *args, **kw)

    @suspend
    def Write_Dirty_Bit(self, *args, **kw):
        return self.cls.Write_Dirty_Bit(self, *args, **kw)


    @suspend
    def Write_Block_Flags(self, *args, **kw):
        return self.cls.Write_Block_Flags(self, *args, **kw)

    @suspend
    def Read_Region_Offsets(self, *args, **kw):
        return self.cls.Read_Region_Offsets(self, *args, **kw)

    @suspend
    def Read_Dirty_Bit(self, *args, **kw):
        return self.cls.Read_Dirty_Bit(self, *args, **kw)

    @suspend
    def Read_Occupancy(self, *args, **kw):
        return self.cls.Read_Occupancy(self, *args, **kw)

    @suspend
    def Read_Block_Flags(self, *args, **kw):
        return self.cls.Read_Block_Flags(self, *args, **kw)

    @suspend
    def Read_Geology(self, *args, **kw):
        return self.cls.Read_Geology(self, *args, **kw)

    @suspend
    def Read_Block_40d(self, *args, **kw):
        return self.cls.Read_Block_40d(self, *args, **kw)

    @suspend
    def get_block(self, point):
        point = point.get_block()
        block = Block(api=self.api, coords=point)
        return block

    @suspend
    def get_tile(self, x=0, y=0, z=0, point=None):
        if not point:
            point = Point(x, y, z)
        block = Block(api=self.api, coords=point.get_block())
        return block.get_tile(point=point)

        
