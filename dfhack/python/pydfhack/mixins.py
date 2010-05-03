# -*- coding: utf-8 -*-
"""
Mix-ins for bound classes
"""

class NeedsStart(object):
    """
    This mixin enforces Start/Finish routines
    """
    started = False
    def prepare(self):
        """
        Enforce Suspend/Start
        """
        if self.api.prepare():
            if not self.started:
                self.Start()
            return self.started
        else:
            return False
        
    def Start(self):
        if self.started:
            return True
        
        if self.api.prepare():
            self.started = self.cls.Start(self)
            if self.started:
                self.api.started.append(self)
            return self.started
        else:
            return False
        
    start = Start
        
    def Finish(self):
        if self.started:
            self.cls.Finish(self)
            self.started = False
            if self in self.api.started:
                self.api.started.remove(self)

    finish = Finish

class NoStart(object):
    """
    This mixin enforces Suspend, and disables Start/Finish
    """
    def prepare(self):
        """
        Enforce Suspend
        """
        return self.api.prepare()
        
    def Start(self):
        raise Exception("{0.cls} does not need Start()".format(self))

    start = Start
        
    def Finish(self):
        raise Exception("{0.cls} does not need Finish()".format(self))

    finish = Finish
