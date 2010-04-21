from pydfhack import *

class API(_API):
    def __init__(self, *args, **kwds):
        _API.__init__(self, args, kwds)

class Map(_MapManager):
    def __init__(self, *args, **kwds):
        _MapManager.__init__(self, args, kwds)

class Vegetation(_VegetationManager):
    def __init__(self, *args, **kwds):
        _VegetationManager.__init__(self, args, kwds)

class GUI(_GUIManager):
    def __init__(self, *args, **kwds):
        _GUIManager.__init__(self, args, kwds)