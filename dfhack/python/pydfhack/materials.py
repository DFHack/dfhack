# -*- coding: utf-8 -*-
"""
Python class for DF_Hack::Materials
"""
from ._pydfhack import _MaterialsManager
class Materials(_MaterialsManager):
    api = None
    started = False
    def __init__(self, api, *args, **kwds):
        _MaterialsManager.__init__(self, args, kwds)
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
                
        

