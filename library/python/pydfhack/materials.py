from ctypes import *
from dftypes import libdfhack
from util import *

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
    
    def _get_inorganic(self):
        self.inorganic = libdfhack.Materials_getInorganic(self._mat_ptr)
    def _get_organic(self):
        self.organic = libdfhack.Materials_getOrganic(self._mat_ptr)
    def _get_tree(self):
        self.tree = libdfhack.Materials_getTree(self._mat_ptr)
    def _get_plant(self):
        self.plant = libdfhack.Materials_getPlant(self._mat_ptr)
    def _get_race(self):
        self.race = libdfhack.Materials_getRace(self._mat_ptr)
    def _get_race_ex(self):
        self.race_ex = libdfhack.Materials_getRaceEx(self._mat_ptr)
    def _get_color(self):
        self.color = libdfhack.Materials_getColor(self._mat_ptr)
    def _get_other(self):
        self.other = libdfhack.Materials_getOther(self._mat_ptr)
    
    def _get_all(self):
        self._get_inorganic()
        self._get_organic()
        self._get_tree()
        self._get_plant()
        self._get_race()
        self._get_race_ex()
        self._get_color()
        self._get_other()
    def _clear_all(self):
        self.inorganic = None
        self.organic = None
        self.tree = None
        self.plant = None
        self.race = None
        self.race_ex = None
        self.color = None
        self.other = None

    def read_inorganic(self):
        result = libdfhack.Materials_ReadInorganicMaterials(self._mat_ptr) > 0
        
        if result == True:
            self._get_inorganic()
        else:
            self.inorganic = None
        
        return result

    def read_organic(self):
        result = libdfhack.Materials_ReadOrganicMaterials(self._mat_ptr) > 0
        
        if result == True:
            self._get_organic()
        else:
            self.organic = None
        
        return result

    def read_tree(self):
        result = libdfhack.Materials_ReadWoodMaterials(self._mat_ptr) > 0
        
        if result == True:
            self._get_tree()
        else:
            self.tree = None
        
        return result

    def read_plant(self):
        result = libdfhack.Materials_ReadPlantMaterials(self._mat_ptr) > 0
        
        if result == True:
            self._get_plant()
        else:
            self.plant = None
        
        return result

    def read_creature_types(self):
        result = libdfhack.Materials_ReadCreatureTypes(self._mat_ptr) > 0
        
        if result == True:
            self._get_race()
        else:
            self.race = None
        
        return result

    def read_creature_types_ex(self):
        result = libdfhack.Materials_ReadCreatureTypesEx(self._mat_ptr) > 0
        
        if result == True:
            self._get_race_ex()
        else:
            self.race_ex = None
        
        return result

    def read_descriptor_colors(self):
        result = libdfhack.Materials_ReadDescriptorColors(self._mat_ptr) > 0
        
        if result == True:
            self._get_color()
        else:
            self.color = None
        
        return result

    def read_others(self):
        result = libdfhack.Materials_ReadOthers(self._mat_ptr) > 0
        
        if result == True:
            self._get_other()
        else:
            self.other = None
        
        return result

    def read_all(self):
        result = libdfhack.Materials_ReadAllMaterials(self._mat_ptr) > 0
        
        if result == True:
            self._get_all()
        else:
            self._clear_all()
        
        return result
    
    def get_type(self, material):
        return libdfhack.Materials_getType(self._mat_ptr, byref(material))

    def get_description(self, material):
        return libdfhack.Materials_getDescription(self._mat_ptr, byref(material))