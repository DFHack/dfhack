# -*- coding: utf-8 -*-
"""
Python class for DF_Hack::Construction
"""
from ._pydfhack import _ConstructionManager
from .mixins import NeedsStart
from .decorators import suspend

class Construction(NeedsStart, _ConstructionManager):
    api = None
    cls = _ConstructionManager
    def __init__(self, api, *args, **kwds):
        self.cls.__init__(self, args, kwds)
        self.api = api

    @suspend
    def Read(self, *args, **kw):
        return self.cls.Read(self, *args, **kw)
