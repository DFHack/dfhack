# -*- coding: utf-8 -*-
"""
Python class for DF_Hack::Vegetation
"""
from ._pydfhack import _VegetationManager
from .mixins import NeedsStart
from .decorators import suspend

class Vegetation(NeedsStart, _VegetationManager):
    api = None
    cls = _VegetationManager
    def __init__(self, api, *args, **kwds):
        self.cls.__init__(self, args, kwds)
        self.api = api
        
    @suspend
    def Read(self, *args, **kw):
        return self.cls.Read(self, *args, **kw)
