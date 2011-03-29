from ctypes import *
from dftypes import *

libdfhack.ContextManager_Alloc.restype = c_void_p
libdfhack.ContextManager_Free.argtypes = [ c_void_p ]

libdfhack.ContextManager_getContext.restype = c_void_p
libdfhack.ContextManager_getSingleContext.restype = c_void_p

libdfhack.Context_Free.argtypes = [ c_void_p ]

libdfhack.Context_getMemoryInfo.restype = c_void_p
libdfhack.Context_getProcess.restype = c_void_p

libdfhack.Context_getCreatures.restype = c_void_p
libdfhack.Context_getMaps.restype = c_void_p
libdfhack.Context_getGui.restype = c_void_p
libdfhack.Context_getMaterials.restype = c_void_p
libdfhack.Context_getTranslation.restype = c_void_p
libdfhack.Context_getVegetation.restype = c_void_p
libdfhack.Context_getBuildings.restype = c_void_p
libdfhack.Context_getConstructions.restype = c_void_p
libdfhack.Context_getItems.restype = c_void_p
libdfhack.Context_getWorld.restype = c_void_p
libdfhack.Context_getWindowIO.restype = c_void_p

class ContextManager(object):
    def __init__(self, memory_path):
        self._cm_ptr = libdfhack.ContextManager_Alloc(create_string_buffer(memory_path))

    def __del__(self):
        libdfhack.ContextManager_Free(self._cm_ptr)

    def refresh(self):
        return libdfhack.ContextManager_Refresh(self._cm_ptr) > 0

    def purge(self):
        libdfhack.ContextManager_purge(self._cm_ptr)

    def get_context(self, index):
        p = libdfhack.ContextManager_getContext(self._cm_ptr, index)

        if p:
            return Context(p)
        else:
            return None

    def get_single_context(self):
        p = libdfhack.ContextManager_getSingleContext(self._cm_ptr)

        if p:
            return Context(p)
        else:
            return None

class Context(object):
    def __init__(self, ptr):
        self._c_ptr = ptr

        self._mat_obj = None
        self._map_obj = None
        self._veg_obj = None
        self._build_obj = None
        self._con_obj = None
        self._gui_obj = None
        self._tran_obj = None
        self._item_obj = None
        self._creature_obj = None
        self._world_obj = None
        self._window_io_obj = None

    def __del__(self):
        libdfhack.Context_Free(self._c_ptr)

    def attach(self):
        return libdfhack.Context_Attach(self._c_ptr) > 0

    def detach(self):
        return libdfhack.Context_Detach(self._c_ptr) > 0

    def suspend(self):
        return libdfhack.Context_Suspend(self._c_ptr) > 0

    def resume(self):
        return libdfhack.Context_Resume(self._c_ptr) > 0

    def force_resume(self):
        return libdfhack.Context_ForceResume(self._c_ptr) > 0

    def async_suspend(self):
        return libdfhack.Context_AsyncSuspend(self._c_ptr) > 0

    @property
    def is_attached(self):
        return libdfhack.Context_isAttached(self._c_ptr) > 0

    @property
    def is_suspended(self):
        return libdfhack.Context_isSuspended(self._c_ptr) > 0

    @property
    def materials(self):
        import materials
        if self._mat_obj is None:
            self._mat_obj = materials.Materials(libdfhack.Context_getMaterials(self._c_ptr))

        return self._mat_obj

    @property
    def maps(self):
        import maps
        if self._map_obj is None:
            self._map_obj = maps.Maps(libdfhack.Context_getMaps(self._c_ptr))

        return self._map_obj

    @property
    def vegetation(self):
        import vegetation
        if self._veg_obj is None:
            self._veg_obj = vegetation.Vegetation(libdfhack.Context_getVegetation(self._c_ptr))

        return self._veg_obj

    @property
    def buildings(self):
        import buildings
        if self._build_obj is None:
            self._build_obj = buildings.Buildings(libdfhack.Context_getBuildings(self._c_ptr))

        return self._build_obj

    @property
    def creatures(self):
        import creatures
        if self._creature_obj is None:
            self._creature_obj = creatures.Creatures(libdfhack.Context_getCreatures(self._c_ptr))

        return self._creature_obj

    @property
    def gui(self):
        import gui
        if self._gui_obj is None:
            self._gui_obj = gui.Gui(libdfhack.Context_getGui(self._c_ptr))

        return self._gui_obj

    @property
    def items(self):
        import items
        if self._item_obj is None:
            self._item_obj = items.Items(libdfhack.Context_getItems(self._c_ptr))

        return self._item_obj

    @property
    def translation(self):
        import translation
        if self._tran_obj is None:
            self._tran_obj = translation.Translation(libdfhack.Context_getTranslation(self._c_ptr))

        return self._tran_obj
    
    @property
    def world(self):
        import world
        if self._world_obj is None:
            self._world_obj = world.World(libdfhack.Context_getWorld(self._c_ptr))
        
        return self._world_obj
    
    @property
    def window_io(self):
        import window_io
        if self._window_io_obj is None:
            self._window_io_obj = window_io.WindowIO(libdfhack.Context_getWindowIO(self._c_ptr))
        
        return self._window_io_obj

cm = ContextManager("Memory.xml")
df = cm.get_single_context()
df.attach()
w = df.window_io