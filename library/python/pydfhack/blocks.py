# -*- coding: utf-8 -*-
"""
Module for high-level abstraction of tiles
"""
from collections import namedtuple
from .pydftypes import Rectangle
from .mixins import NoStart
from .decorators import suspend

class Point(object):
    x = None
    y = None
    z = None
    block = False
    def __init__(self, x, y, z, block=False):
        self.x = x
        self.y = y
        self.z = z
        self.block = block
    
    def get_block(self):
        if not self.block:
            return Point(self.x/16, self.y/16, self.z, True)
        else:
            return self

    @property
    def xyz(self):
        return (self.x, self.y, self.z)

    @property
    def rect(self):
        """
        Return block coords in form (x1, y1, x2, y2)
        """
        block = self.get_block()
        x1 = block.x * 16
        y1 = block.y * 16
        return Rectangle(x1, y1, x1+15, y1+15)
        
    
    def __repr__(self):
        b = ''
        if self.block:
            b = ', Block'
        return "<Point(X:{0.x}, Y:{0.y}, Z:{0.z}{1})>".format(self, b)
       
class Block(NoStart):
    """
    16x16 tiles block
    """
    api = None
    tiles = None
    coords = None
    x1 = None
    y1 = None
    def __init__(self, api, coords):
        """
        api is instance of API, which is used for read/write operations
        coords is Point object
        """
        self.api = api
        if not isinstance(coords, Point):
            raise Exception(u"coords parameter should be Point")
            
        coords = coords.get_block()
        self.coords = coords
        self.rect   = self.coords.rect
        self.reload()

    @suspend
    def reload(self):
        tile_types = self.api.maps.Read_Tile_Types(point=self.coords)
        tile_flags = self.api.maps.Read_Designations(point=self.coords)
        if self.tiles:
            for x in range(16):
                for y in range(16):
                    self.tiles[x][y].update(ttype=tile_types[x][y], flags=tile_flags[x][y])
        else:
            self.tiles = []
            for x in range(16):
                row = []
                for y in range(16):
                    row.append(Tile(self, tile_types[x][y], tile_flags[x][y], self.rect.x1 + x, self.rect.y1 + y, self.coords.z))
                self.tiles.append(row)
        
    @suspend
    def save(self):
        tile_types = map(lambda x:range(16), range(16))
        tile_flags = map(lambda x:range(16), range(16))
        for x in range(16):
            for y in range(16):
                tile_types[x][y] = self.tiles[x][y].ttype
                tile_flags[x][y] = self.tiles[x][y].flags
        self.api.maps.Write_Tile_Types(point=self.coords, tiles=tile_types)
        # Designations are bugged
        
        #self.api.maps.Write_Designations(point=self.coords, tiles=tile_flags)

    def get_tile(self, x=0, y=0, z=0, point=None):
        if point:
            x, y, z = point.xyz

        if z == self.coords.z and (self.rect.y1 <= y <= self.rect.y2) and (self.rect.x1 <= x <= self.rect.x2):
            return self.tiles[x%16][y%16]
        else:
            raise Exception("This tile does not belong to this block")

    def __repr__(self):
        return "<Block(X:{2}-{4}, Y:{3}-{5}, Z:{1.z})>".format(self, self.coords, *self.coords.rect)
        


class Tile(object):
    coords = None
    block  = None
    ttype  = None
    temperature = None
    flags  = None
    def __init__(self, block, ttype, flags, x=0, y=0, z=0, point=None):
        self.ttype = ttype
        self.flags = flags
        if not point:
            point = Point(x, y, z)
        self.coords = point
        self.block = block

    def update(self, ttype, flags):
        self.ttype = ttype
        self.flags = flags

    def reload(self):
        self.block.reload()
        
    def save(self):
        self.block.save()

    def __repr__(self):
        return "<Tile({0.ttype} at {1.x}, {1.y}, {1.z})>".format(self, self.coords)
