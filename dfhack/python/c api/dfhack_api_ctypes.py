from ctypes import *
from pydftypes import *


libdfhack.API_Alloc.restype = c_void_p
libdfhack.API_Free.argtypes = [ c_void_p ]

libdfhack.API_getMemoryInfo.restype = c_void_p
libdfhack.API_getProcess.restype = c_void_p
libdfhack.API_getWindow.restype = c_void_p

libdfhack.API_getCreatures.restype = c_void_p
libdfhack.API_getMaps.restype = c_void_p
libdfhack.API_getGui.restype = c_void_p
libdfhack.API_getPosition.restype = c_void_p
libdfhack.API_getMaterials.restype = c_void_p
libdfhack.API_getTranslation.restype = c_void_p
libdfhack.API_getVegetation.restype = c_void_p
libdfhack.API_getBuildings.restype = c_void_p
libdfhack.API_getConstructions.restype = c_void_p
libdfhack.API_getItems.restype = c_void_p

class API(object):
    def __init__(self, memory_path):
        self._api_ptr = libdfhack.API_Alloc(create_string_buffer(memory_path))

        self._pos_obj = None
        self._mat_obj = None
        self._map_obj = None
        self._veg_obj = None
        self._build_obj = None
        self._con_obj = None
        self._gui_obj = None
        self._tran_obj = None
        self._item_obj = None
        self._creature_obj = None

    def __del__(self):
        libdfhack.API_Free(self._api_ptr)

    def attach(self):
        return libdfhack.API_Attach(self._api_ptr) > 0

    def detach(self):
        return libdfhack.API_Detach(self._api_ptr) > 0

    def suspend(self):
        return libdfhack.API_Suspend(self._api_ptr) > 0

    def resume(self):
        return libdfhack.API_Resume(self._api_ptr) > 0

    def force_resume(self):
        return libdfhack.API_ForceResume(self._api_ptr) > 0

    def async_suspend(self):
        return libdfhack.API_AsyncSuspend(self._api_ptr) > 0

    @property
    def is_attached(self):
        return libdfhack.API_isAttached(self._api_ptr) > 0

    @property
    def is_suspended(self):
        return libdfhack.API_isSuspended(self._api_ptr) > 0

    @property
    def position(self):
        import position
        if self._pos_obj is None:
            self._pos_obj = position.Position(libdfhack.API_getPosition(self._api_ptr))

        return self._pos_obj

    @property
    def materials(self):
        import materials
        if self._mat_obj is None:
            self._mat_obj = materials.Materials(libdfhack.API_getMaterials(self._api_ptr))

        return self._mat_obj

    @property
    def maps(self):
        import maps
        if self._map_obj is None:
            self._map_obj = maps.Maps(libdfhack.API_getMaps(self._api_ptr))

        return self._map_obj

    @property
    def vegetation(self):
        import vegetation
        if self._veg_obj is None:
            self._veg_obj = vegetation.Vegetation(libdfhack.API_getVegetation(self._api_ptr))

        return self._veg_obj

    @property
    def buildings(self):
        import buildings
        if self._build_obj is None:
            self._build_obj = buildings.Buildings(libdfhack.API_getBuildings(self._api_ptr))

        return self._build_obj

    @property
    def creatures(self):
        import creatures
        if self._creature_obj is None:
            self._creature_obj = creatures.Creatures(libdfhack.API_getCreatures(self._api_ptr))

        return self._creature_obj

    @property
    def gui(self):
        import gui
        if self._gui_obj is None:
            self._gui_obj = gui.Gui(libdfhack.API_getGui(self._api_ptr))

        return self._gui_obj

    @property
    def items(self):
        import items
        if self._item_obj is None:
            self._item_obj = items.Items(libdfhack.API_getItems(self._api_ptr))

        return self._item_obj

    @property
    def translation(self):
        import translation
        if self._tran_obj is None:
            self._tran_obj = translation.Translation(libdfhack.API_getTranslation(self._api_ptr))

        return self._tran_obj

def reveal():
    df = API("Memory.xml")
    df.attach()

    m = df.maps

    m.start()

    m_x, m_y, m_z = m.size

    for x in xrange(m_x):
        for y in xrange(m_y):
            for z in xrange(m_z):
                if m.is_valid_block(x, y, z):
                    d = m.read_designations(x, y, z)

                    for i in d:
                        for j in i:
                            j.bits.hidden = 0

                    m.write_designations(x, y, z, d)

    m.finish()
    df.detach()
