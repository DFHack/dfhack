# -*- coding: utf-8 -*-
"""
Decorators for bound classes
"""
from decorator import decorator

@decorator
def suspend(func, self, *args, **kw):
    """
    This decorator will try to suspend DF and start needed module before running func.
    If DF was resumed when decorator was run, it will try to resume back after executing func
    """
    susp = not self.api.is_attached or self.api.is_suspended
    if self.prepare():
        res = func(self, *args, **kw)
        if not susp:
            self.api.Resume()
        return res
    else:
        raise Exception(u"Could not suspend/start")
