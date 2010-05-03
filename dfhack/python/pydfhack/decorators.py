# -*- coding: utf-8 -*-
"""
Decorators for bound classes
"""
from decorator import decorator

@decorator
def suspend(func, self, *args, **kw):
    """
    This decorator will try to suspend DF and start needed module before running func
    """
    if self.prepare():
        return func(self, *args, **kw)
    else:
        raise Exception(u"Could not suspend/start")
