# -*- coding: utf-8 -*-
"""
Python class for DF_Hack::Translation
"""
from ._pydfhack import _TranslationManager
from .mixins import NeedsStart
from .decorators import suspend

class Translation(NeedsStart, _TranslationManager):
    api = None
    cls = _TranslationManager
    def __init__(self, api, *args, **kwds):
        self.cls.__init__(self, args, kwds)
        self.api = api

    def get_dictionaries(self):
        return self.dictionaries

    def Translate_Name(self, *args, **kw):
        return self.cls.Translate_Name(self, *args, **kw)
