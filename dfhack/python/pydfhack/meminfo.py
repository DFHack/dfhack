# -*- coding: utf-8 -*-
"""
Python class for DF_Hack::MemInfo
"""
from ._pydfhack import _MemInfo
class MemInfo(_MemInfo):
    api = None
    started = False
    def __init__(self, api, *args, **kwds):
        _MemInfo.__init__(self, args, kwds)
        self.api = api

    def prepare(self):
        """
        Enforce Suspend/Start
        """
        if self.api.prepare():
            if not self.started:
                self.started = self.Start()
            return self.started
        else:
            return False
                
        

