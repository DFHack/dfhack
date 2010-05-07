from ctypes import *
from pydftypes import *

int_ptr = POINTER(c_int)
uint_ptr = POINTER(c_uint)

def _uintify(x, y, z):
    return (c_uint(x), c_uint(y), c_uint(z))

libdfhack = cdll.libdfhack
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

class API(object):
    def __init__(self, memory_path):
        self._api_ptr = libdfhack.API_Alloc(create_string_buffer(memory_path))

        self._pos_obj = None
        self._mat_obj = None
        self._map_obj = None

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
        if self._pos_obj is None:
            ptr = libdfhack.API_getPosition(self._api_ptr)

            if ptr is not None:
                return Position(ptr)
            else:
                return None
        else:
            return self._pos_obj

    @property
    def materials(self):
        if self._mat_obj is None:
            ptr = libdfhack.API_getMaterials(self._api_ptr)

            if ptr is not None:
                return Materials(ptr)
            else:
                return None
        else:
            return self._mat_obj

    @property
    def maps(self):
        if self._map_obj is None:
            ptr = libdfhack.API_getMaps(self._api_ptr)

            if ptr is not None:
                return Maps(ptr)
            else:
                return None
        else:
            return self._map_obj

class Position(object):
    def __init__(self, ptr):
        self._pos_ptr = ptr

        self._vx, self._vy, self._vz = c_int(), c_int(), c_int()
        self._cx, self._cy, self._cz = c_int(), c_int(), c_int()
        self._ww, self._wh = c_int(), c_int()

    def get_view_coords(self):
        if libdfhack.Position_getViewCoords(self._pos_ptr, byref(self._vx), byref(self._vy), byref(self._vz)) > 0:
            return (self._vx.value, self._vy.value, self._vz.value)
        else:
            return (-1, -1, -1)

    def set_view_coords(self, v_coords):
        self._vx.value, self._vy.value, self._vz.value = v_coords

        libdfhack.Position_setViewCoords(self._pos_ptr, self._vx, self._vy, self._vz)

    view_coords = property(get_view_coords, set_view_coords)
    
    def get_cursor_coords(self):
        if libdfhack.Position_getCursorCoords(self._pos_ptr, byref(self._cx), byref(self._cy), byref(self._cz)) > 0:
            return (self._cx.value, self._cy.value, self._cz.value)
        else:
            return (-1, -1, -1)

    def set_cursor_coords(self, c_coords):
        self._cx.value, self._cy.value, self_cz.value = c_coords

        libdfhack.Position_setCursorCoords(self._pos_ptr, self._cx, self._cy, self._cz)

    cursor_coords = property(get_cursor_coords, set_cursor_coords)

    @property
    def window_size(self):
        if libdfhack.Position_getWindowSize(self._pos_ptr, byref(self._ww), byref(self._wh)) > 0:
            return (self._ww.value, self._wh.value)
        else:
            return (-1, -1)

libdfhack.Gui_ReadViewScreen.argtypes = [ c_void_p, c_void_p ]

class Gui(object):
    def __init__(self, ptr):
        self._gui_ptr = ptr

    def start(self):
        return libdfhack.Gui_Start(self._gui_ptr)

    def finish(self):
        return libdfhack.Gui_Finish(self._gui_ptr)

    def read_pause_state(self):
        return libdfhack.Gui_ReadPauseState(self._pos_ptr) > 0

    def read_view_screen(self):
        s = ViewScreen()

        if libdfhack.Gui_ReadViewScreen(self._gui_ptr, byref(s)) > 0:
            return s
        else:
            return None

arr_create_func = CFUNCTYPE(c_void_p, c_int)

get_arg_types = [ c_void_p, arr_create_func ]

libdfhack.Materials_getInorganic.argtypes = get_arg_types
libdfhack.Materials_getOrganic.argtypes = get_arg_types
libdfhack.Materials_getTree.argtypes = get_arg_types
libdfhack.Materials_getPlant.argtypes = get_arg_types
libdfhack.Materials_getRace.argtypes = get_arg_types
#libdfhack.Materials_getRaceEx.argtypes = get_arg_types
libdfhack.Materials_getColor.argtypes = get_arg_types
libdfhack.Materials_getOther.argtypes = get_arg_types

class Materials(object):
    def __init__(self, ptr):
        self._mat_ptr = ptr

        self.inorganic = None
        self.organic = None
        self.tree = None
        self.plant = None
        self.race = None
        self.race_ex = None
        self.color = None
        self.other = None

    def read_inorganic(self):
        return libdfhack.Materials_ReadInorganicMaterials(self._mat_ptr)

    def read_organic(self):
        return libdfhack.Materials_ReadOrganicMaterials(self._mat_ptr)

    def read_wood(self):
        return libdfhack.Materials_ReadWoodMaterials(self._mat_ptr)

    def read_plant(self):
        return libdfhack.Materials_ReadPlantMaterials(self._mat_ptr)

    def read_creature_types(self):
        return libdfhack.Materials_ReadCreatureTypes(self._mat_ptr)

    def read_creature_types_ex(self):
        return libdfhack.Materials_ReadCreatureTypesEx(self._mat_ptr)

    def read_descriptor_colors(self):
        return libdfhack.Materials_ReadDescriptorColors(self._mat_ptr)

    def read_others(self):
        return libdfhack.Materials_ReadOthers(self._mat_ptr)

    def read_all(self):
        libdfhack.Materials_ReadAllMaterials(self._mat_ptr)

    def get_description(self, material):
        return libdfhack.Materials_getDescription(self._mat_ptr, byref(material))

    def _allocate_matgloss_array(self, count):
        arr_type = Matgloss * count

        arr = arr_type()

        ptr = c_void_p()
        ptr = addressof(arr)

        return (arr, ptr)

    def update_inorganic_cache(self):
        def update_callback(count):
            allocated = self._allocate_matgloss_array(count)

            self.inorganic = allocated[0]

            return allocated[1]

        callback = arr_create_func(update_callback)

        return libdfhack.Materials_getInorganic(self._mat_ptr, callback)

    def update_organic_cache(self):
        def update_callback(count):
            allocated = self._allocate_matgloss_array(count)

            self.organic = allocated[0]

            return allocated[1]

        callback = arr_create_func(update_callback)

        return libdfhack.Materials_getOrganic(self._mat_ptr, callback)

    def update_tree_cache(self):
        def update_callback(count):
            allocated = self._allocate_matgloss_array(count)

            self.tree = allocated[0]

            return allocated[1]

        callback = arr_create_func(update_callback)

        return libdfhack.Materials_getTree(self._mat_ptr, callback)

    def update_plant_cache(self):
        def update_callback(count):
            allocated = self._allocate_matgloss_array(count)

            self.plant = allocated[0]

            return allocated[1]

        callback = arr_create_func(update_callback)

        return libdfhack.Materials_getPlant(self._mat_ptr, callback)

    def update_race_cache(self):
        def update_callback(count):
            allocated = self._allocate_matgloss_array(count)

            self.race = allocated[0]

            return allocated[1]

        callback = arr_create_func(update_callback)

        return libdfhack.Materials_getRace(self._mat_ptr, callback)

libdfhack.Maps_getSize.argtypes = [ c_void_p, uint_ptr, uint_ptr, uint_ptr ]
libdfhack.Maps_ReadTileTypes.argtypes = [ c_void_p, c_uint, c_uint, c_uint, POINTER(TileTypes40d) ]
libdfhack.Maps_WriteTileTypes.argtypes = [ c_void_p, c_uint, c_uint, c_uint, POINTER(TileTypes40d) ]
libdfhack.Maps_ReadDesignations.argtypes = [ c_void_p, c_uint, c_uint, c_uint, POINTER(Designations40d) ]
libdfhack.Maps_WriteDesignations.argtypes = [ c_void_p, c_uint, c_uint, c_uint, POINTER(Designations40d) ]
libdfhack.Maps_ReadTemperatures.argtypes = [ c_void_p, c_uint, c_uint, c_uint, POINTER(Temperatures) ]
libdfhack.Maps_WriteTemperatures.argtypes = [ c_void_p, c_uint, c_uint, c_uint, POINTER(Temperatures) ]
libdfhack.Maps_ReadOccupancy.argtypes = [ c_void_p, c_uint, c_uint, c_uint, POINTER(Occupancies40d) ]
libdfhack.Maps_WriteOccupancy.argtypes = [ c_void_p, c_uint, c_uint, c_uint, POINTER(Occupancies40d) ]
libdfhack.Maps_ReadRegionOffsets.argtypes = [ c_void_p, c_uint, c_uint, c_uint, POINTER(BiomeIndices40d) ]

class Maps(object):
    def __init__(self, ptr):
        self._map_ptr = ptr

    def start(self):
        return libdfhack.Maps_Start(self._map_ptr) > 0

    def finish(self):
        return libdfhack.Maps_Finish(self._map_ptr) > 0

    def is_valid_block(self, x, y, z):
        return libdfhack.Maps_isValidBlock(self._map_ptr, *_uintify(x, y, z)) > 0

    def read_tile_types(self, x, y, z):
        tt = TileTypes40d()

        if libdfhack.Maps_ReadTileTypes(self._map_ptr, *_uintify(x, y, z), tt) > 0:
            return tt
        else:
            return None

    def write_tile_types(self, x, y, z, tt):
        return libdfhack.Maps_WriteTileTypes(self._map_ptr, *_uintify(x, y, z), tt) > 0

    def read_designations(self, x, y, z):
        d = Designations40d()

        if libdfhack.Maps_ReadDesignations(self._map_ptr, *_uintify(x, y, z), d) > 0:
            return d
        else:
            return None

    def write_designations(self, x, y, z, d):
        return libdfhack.Maps_WriteDesignations(self._map_ptr, *_uintify(x, y, z), d) > 0

    def read_temperatures(self, x, y, z):
        t = Temperatures()

        if libdfhack.Maps_ReadDesignations(self._map_ptr, *_uintify(x, y, z), t) > 0:
            return t
        else:
            return None

    def write_temperatures(self, x, y, z, t):
        return libdfhack.Maps_WriteDesignations(self._map_ptr, *_uintify(x, y, z), t) > 0

    def read_occupancy(self, x, y, z):
        o = Occupancies40d()

        if libdfhack.Maps_ReadDesignations(self._map_ptr, *_uintify(x, y, z), o) > 0:
            return o
        else:
            return None

    def write_designations(self, x, y, z, o):
        return libdfhack.Maps_WriteDesignations(self._map_ptr, *_uintify(x, y, z), o) > 0

    def read_dirty_bit(self, x, y, z):
        bit = c_int(0)

        if libdfhack.Maps_ReadDirtyBit(self._map_ptr, *_uintify(x, y, z), byref(bit)) > 0:
            if bit > 0:
                return True
            else:
                return False
        else:
            return None

    def write_dirty_bit(self, x, y, z, dirty):
        return libdfhack.Maps_WriteDirtyBit(self._map_ptr, *_uintify(x, y, z), c_int(dirty)) > 0

    @property
    def size(self):
        x = c_uint()
        y = c_uint()
        z = c_uint()

        retval = libdfhack.Maps_getSize(self._map_ptr, byref(x), byref(y), byref(z))

        if retval > 0:
            return (int(x.value), int(y.value), int(z.value))
        else:
            return (-1, -1, -1)

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
