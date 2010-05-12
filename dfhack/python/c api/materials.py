from ctypes import *
from pydftypes import libdfhack
from util import *

_get_arg_types = [ c_void_p, _arr_create_func ]

libdfhack.Materials_getInorganic.argtypes = _get_arg_types
libdfhack.Materials_getOrganic.argtypes = _get_arg_types
libdfhack.Materials_getTree.argtypes = _get_arg_types
libdfhack.Materials_getPlant.argtypes = _get_arg_types
libdfhack.Materials_getRace.argtypes = _get_arg_types
#libdfhack.Materials_getRaceEx.argtypes = _get_arg_types
libdfhack.Materials_getColor.argtypes = _get_arg_types
libdfhack.Materials_getOther.argtypes = _get_arg_types

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

    def update_inorganic_cache(self):
        def update_callback(count):
            allocated = _allocate_array(Matgloss, count)

            self.inorganic = allocated[0]

            return allocated[1]

        callback = _arr_create_func(update_callback)

        return libdfhack.Materials_getInorganic(self._mat_ptr, callback)

    def update_organic_cache(self):
        def update_callback(count):
            allocated = _allocate_array(Matgloss, count)

            self.organic = allocated[0]

            return allocated[1]

        callback = _arr_create_func(update_callback)

        return libdfhack.Materials_getOrganic(self._mat_ptr, callback)

    def update_tree_cache(self):
        def update_callback(count):
            allocated = _allocate_array(Matgloss, count)

            self.tree = allocated[0]

            return allocated[1]

        callback = _arr_create_func(update_callback)

        return libdfhack.Materials_getTree(self._mat_ptr, callback)

    def update_plant_cache(self):
        def update_callback(count):
            allocated = _allocate_array(Matgloss, count)

            self.plant = allocated[0]

            return allocated[1]

        callback = _arr_create_func(update_callback)

        return libdfhack.Materials_getPlant(self._mat_ptr, callback)

    def update_race_cache(self):
        def update_callback(count):
            allocated = _allocate_array(Matgloss, count)

            self.race = allocated[0]

            return allocated[1]

        callback = _arr_create_func(update_callback)

        return libdfhack.Materials_getRace(self._mat_ptr, callback)

    def update_color_cache(self):
        def update_callback(count):
            allocated = _allocate_array(DescriptorColor, count)

            self.color = allocated[0]
            
            return allocated[1]

        callback = _arr_create_func(update_callback)

        return libdfhack.Materials_getColor(self._mat_ptr, callback)

    def update_other_cache(self):
        def update_callback(count):
            allocated = _allocate_array(MatglossOther, count)

            self.other = allocated[0]
            
            return allocated[1]

        callback = _arr_create_func(update_callback)

        return libdfhack.Materials_getOther(self._mat_ptr, callback)
