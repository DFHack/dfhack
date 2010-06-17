# -*- coding: utf-8 -*-
"""
Python class for DF_Hack::Materials
"""
from ._pydfhack import _MaterialsManager
from .mixins import NoStart
from .decorators import suspend

class Materials(NoStart, _MaterialsManager):
    api = None
    cls = _MaterialsManager
    def __init__(self, api, *args, **kwds):
        cls.__init__(self, args, kwds)
        self.api = api

    @suspend
    def Read_Wood_Materials(self, *args, **kw):
        return self.cls.Read_Wood_Materials(self, *args, **kw)

    @suspend
    def Read_Plant_Materials(self, *args, **kw):
        return self.cls.Read_Plant_Materials(self, *args, **kw)

    @suspend
    def Read_Inorganic_Materials(self, *args, **kw):
        return self.cls.Read_Inorganic_Materials(self, *args, **kw)

    @suspend
    def Read_Descriptor_Colors(self, *args, **kw):
        return self.cls.Read_Descriptor_Colors(self, *args, **kw)

    @suspend
    def Read_Creature_Types(self, *args, **kw):
        return self.cls.Read_Creature_Types(self, *args, **kw)

    @suspend
    def Read_Organic_Materials(self, *args, **kw):
        return self.cls.Read_Organic_Materials(self, *args, **kw)

    @suspend
    def Read_Creature_Types_Ex(self, *args, **kw):
        return self.cls.Read_Creature_Types_Ex(self, *args, **kw)
