# -*- coding: utf-8 -*-
"""
Python class for DF_Hack::MemInfo
"""
from ._pydfhack import _MemInfo
from .mixins import NoStart
from .decorators import suspend

class MemInfo(NoStart, _MemInfo):
    api = None
    cls = _MemInfo
    def __init__(self, api, *args, **kwds):
        self.cls.__init__(self, args, kwds)
        self.api = api

    ## Properties:
    # version
    # base

    ## No idea if these should work only on suspend. To check later.
    
    def Rebase_All(self, *args, **kw):
        return self.cls.Rebase_All(self, *args, **kw)
    
    def Set_String(self, *args, **kw):
        return self.cls.Set_String(self, *args, **kw)
    
    def Resolve_Object_To_Class_ID(self, *args, **kw):
        return self.cls.Resolve_Object_To_Class_ID(self, *args, **kw)
    
    def Get_Trait(self, *args, **kw):
        return self.cls.Get_Trait(self, *args, **kw)
    
    def Get_Job(self, *args, **kw):
        return self.cls.Get_Job(self, *args, **kw)
    
    def Rebase_VTable(self, *args, **kw):
        return self.cls.Rebase_VTable(self, *args, **kw)
    
    def Get_String(self, *args, **kw):
        return self.cls.Get_String(self, *args, **kw)
    
    def Get_Trait_Name(self, *args, **kw):
        return self.cls.Get_Trait_Name(self, *args, **kw)
    
    def Set_Job(self, *args, **kw):
        return self.cls.Set_Job(self, *args, **kw)
    
    def Set_Offset(self, *args, **kw):
        return self.cls.Set_Offset(self, *args, **kw)
    
    def Rebase_Addresses(self, *args, **kw):
        return self.cls.Rebase_Addresses(self, *args, **kw)
    
    def Resolve_Classname_To_Class_ID(self, *args, **kw):
        return self.cls.Resolve_Classname_To_Class_ID(self, *args, **kw)
    
    def Set_Labor(self, *args, **kw):
        return self.cls.Set_Labor(self, *args, **kw)
    
    def Get_Skill(self, *args, **kw):
        return self.cls.Get_Skill(self, *args, **kw)
    
    def Set_Profession(self, *args, **kw):
        return self.cls.Set_Profession(self, *args, **kw)
    
    def Get_Profession(self, *args, **kw):
        return self.cls.Get_Profession(self, *args, **kw)
    
    def Get_Offset(self, *args, **kw):
        return self.cls.Get_Offset(self, *args, **kw)
    
    def Get_HexValue(self, *args, **kw):
        return self.cls.Get_HexValue(self, *args, **kw)
    
    def Set_Address(self, *args, **kw):
        return self.cls.Set_Address(self, *args, **kw)
    
    def Set_HexValue(self, *args, **kw):
        return self.cls.Set_HexValue(self, *args, **kw)
    
    def Get_Address(self, *args, **kw):
        return self.cls.Get_Address(self, *args, **kw)
    
    def Set_Trait(self, *args, **kw):
        return self.cls.Set_Trait(self, *args, **kw)
    
    def Resolve_Classname_To_VPtr(self, *args, **kw):
        return self.cls.Resolve_Classname_To_VPtr(self, *args, **kw)
    
    def Get_Labor(self, *args, **kw):
        return self.cls.Get_Labor(self, *args, **kw)
    
    def Set_Skill(self, *args, **kw):
        return self.cls.Set_Skill(self, *args, **kw)
    
    def Resolve_Class_ID_To_Classname(self, *args, **kw):
        return self.cls.Resolve_Class_ID_To_Classname(self, *args, **kw)
