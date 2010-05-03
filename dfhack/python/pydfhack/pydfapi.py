import _pydfhack, os, copy
from .blocks import Point, Block
from .meminfo import MemInfo
from .position import Position
from .materials import Materials
from .creature import Creature
from .map import Map
from .translation import Translation
from .construction import Construction
from .vegetation import Vegetation
from .gui import GUI
class API(_pydfhack._API):
    started = None
    
    for file in ["Memory.xml", os.path.join("..","..","output","Memory.xml")]:
        if os.path.isfile(file):
            datafile = file
            break
    else:
        raise ImportError, "Memory.xml not found."
	
    def prepare(self):
        """
        Enforce Attach/Suspend behavior.
        Return True if succeeded, else False
        """
        r = True
        if not self.is_attached:
            r = self.Attach()
        if r and not self.is_suspended:
            r = self.Suspend()
        return r
	
    def __init__(self, *args, **kwds):
        _pydfhack._API.__init__(self, API.datafile)
        self._mem_info_mgr_type = MemInfo
        self._position_mgr_type = Position
        self._material_mgr_type = Materials
        self._creature_mgr_type = Creature
        self._map_mgr_type = Map
        self._translate_mgr_type = Translation
        self._construction_mgr_type = Construction
        self._vegetation_mgr_type = Vegetation
        self._gui_mgr_type = GUI
        self.started = []

    def Attach(self, *args, **kw):
        print "API.Attach()"
        return _pydfhack._API.Attach(self, *args, **kw)

    def Detach(self, *args, **kw):
        print "API.Detach()"
        return _pydfhack._API.Detach(self, *args, **kw)

    def Suspend(self, *args, **kw):
        print "API.Suspend()"
        return _pydfhack._API.Suspend(self, *args, **kw)
        
    def Resume(self):
        print "API.Resume()"
        # Reference counting is fcked, so leave it alone for now
        
        ## Explicitly Finish() all started modules
        #print self.started
        #started = copy.copy(self.started)
        #print started
        #for m in started:
        #    print m
        #    m.Finish()
        #self.started = []
        return _pydfhack._API.Resume(self)
