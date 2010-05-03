# -*- coding: utf-8 -*-
"""
Python class for DF_Hack::Vegetation
"""
from ._pydfhack import _VegetationManager
class Vegetation(_VegetationManager):
    api = None
    started = False
    def __init__(self, api, *args, **kwds):
        _VegetationManager.__init__(self, args, kwds)
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
                
        

