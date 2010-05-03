# -*- coding: utf-8 -*-
"""
Python class for DF_Hack::Creature
"""
from ._pydfhack import _CreatureManager
from .mixins import NeedsStart
from .decorators import suspend

class Creature(NeedsStart, _CreatureManager):
    api = None
    cls = _CreatureManager
    def __init__(self, api, *args, **kwds):
        self.cls.__init__(self, args, kwds)
        self.api = api

    @suspend
    def Read_Creature(self, *args, **kw):
        return self.cls.Read_Creature(self, *args, **kw)
    
    @suspend
    def Get_Dwarf_Race_Index(self, *args, **kw):
        return self.cls.Get_Dwarf_Race_Index(self, *args, **kw)
    
    @suspend
    def Write_Labors(self, *args, **kw):
        return self.cls.Write_Labors(self, *args, **kw)
    
    @suspend
    def Read_Creature_In_Box(self, *args, **kw):
        return self.cls.Read_Creature_In_Box(self, *args, **kw)
    
    @suspend
    def Get_Dwarf_Civ_id(self, *args, **kw):
        return self.cls.Get_Dwarf_Civ_id(self, *args, **kw)
