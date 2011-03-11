from ctypes import *
from flags import *
from enum import *
import util
from util import *

libdfhack = cdll.libdfhack

def _register_callback(name, func):
    ptr = c_void_p.in_dll(libdfhack, name)
    ptr.value = cast(func, c_void_p).value

_register_callback("alloc_byte_buffer_callback", alloc_byte_buffer)
_register_callback("alloc_ubyte_buffer_callback", alloc_ubyte_buffer)
_register_callback("alloc_short_buffer_callback", alloc_short_buffer)
_register_callback("alloc_ushort_buffer_callback", alloc_ushort_buffer)
_register_callback("alloc_int_buffer_callback", alloc_int_buffer)
_register_callback("alloc_uint_buffer_callback", alloc_uint_buffer)
_register_callback("alloc_char_buffer_callback", alloc_char_buffer)

_arr_create_func = CFUNCTYPE(c_void_p, c_int)
_dfhack_string = (c_char * 128)

TileTypes40d = ((c_int * 16) * 16)
BiomeIndices40d = c_ubyte * 16
Temperatures = ((c_ushort * 16) * 16)
Designations40d = ((DesignationFlags * 16) * 16)
Occupancies40d = ((OccupancyFlags * 16) * 16)

def wall_terrain_check(terrain):
    return libdfhack.DFHack_isWallTerrain(terrain) > 0

def floor_terrain_check(terrain):
    return libdfhack.DFHack_isFloorTerrain(terrain) > 0

def ramp_terrain_check(terrain):
    return libdfhack.DFHack_isRampTerrain(terrain) > 0

def stair_terrain_check(terrain):
    return libdfhack.DFHack_isStairTerrain(terrain) > 0

def open_terrain_check(terrain):
    return libdfhack.DFHack_isOpenTerrain(terrain) > 0

def get_vegetation_type(terrain):
    return libdfhack.DFHack_getVegetationType(terrain)

class Position2D(Structure):
    _fields_ = [("x", c_ushort),
                ("y", c_ushort)]

class PlaneCoord(Union):
    _fields_ = [("xy", c_uint),
                ("dim", Position2D)]

    def __cmp__(self, other):
        if isinstance(other, PlaneCoord):
            return self.xy - other.xy
        else:
            raise TypeError("argument must be of type %s" % type(self))

class Feature(Structure):
    _fields_ = [("type", FeatureType),
                ("main_material", c_short),
                ("sub_material", c_short),
                ("discovered", c_byte),
                ("origin", c_uint)]

class Vein(Structure):
    _fields_ = [("vtable", c_uint),
                ("type", c_int),
                ("assignment", c_short * 16),
                ("flags", c_uint),
                ("address_of", c_uint)]

_vein_ptr = POINTER(Vein)

def _alloc_vein_buffer_callback(ptr, count):
    return util._allocate_array(ptr, Vein, count)

_vein_functype = CFUNCTYPE(c_int, POINTER(_vein_ptr), c_uint)
_vein_func = _vein_functype(_alloc_vein_buffer_callback)
_register_callback("alloc_vein_buffer_callback", _vein_func)

class FrozenLiquidVein(Structure):
    _fields_ = [("vtable", c_uint),
                ("tiles", TileTypes40d),
                ("address_of", c_uint)]

_frozenvein_ptr = POINTER(FrozenLiquidVein)

def _alloc_frozenliquidvein_buffer_callback(ptr, count):
    return util._allocate_array(ptr, FrozenLiquidVein, count)

_frozenliquidvein_functype = CFUNCTYPE(c_int, POINTER(_frozenvein_ptr), c_uint)
_frozenliquidvein_func = _frozenliquidvein_functype(_alloc_frozenliquidvein_buffer_callback)
_register_callback("alloc_frozenliquidvein_buffer_callback", _frozenliquidvein_func)

class SpatterVein(Structure):
    _fields_ = [("vtable", c_uint),
                ("mat1", c_ushort),
                ("unk1", c_ushort),
                ("mat2", c_uint),
                ("mat3", c_ushort),
                ("intensity", ((c_ubyte * 16) * 16)),
                ("address_of", c_uint)]

_spattervein_ptr = POINTER(SpatterVein)

def _alloc_spattervein_buffer_callback(ptr, count):
    return util._allocate_array(ptr, SpatterVein, count)

_spattervein_functype = CFUNCTYPE(c_int, POINTER(_spattervein_ptr), c_uint)
_spattervein_func = _spattervein_functype(_alloc_spattervein_buffer_callback)
_register_callback("alloc_spattervein_buffer_callback", _spattervein_func)

class GrassVein(Structure):
    _fields_ = [("vtable", c_uint),
                ("material", c_uint),
                ("intensity", ((c_ubyte * 16) * 16)),
                ("address_of", c_uint)]

_grassvein_ptr = POINTER(GrassVein)

def _alloc_grassvein_buffer_callback(ptr, count):
    return util._allocate_array(ptr, GrassVein, count)

_grassvein_functype = CFUNCTYPE(c_int, POINTER(_grassvein_ptr), c_uint)
_grassvein_func = _grassvein_functype(_alloc_grassvein_buffer_callback)
_register_callback("alloc_grassvein_buffer_callback", _grassvein_func)

class WorldConstruction(Structure):
    _fields_ = [("vtable", c_uint),
                ("material", c_uint),
                ("assignment", c_ushort * 16),
                ("address_of", c_uint)]

_worldconstruction_ptr = POINTER(WorldConstruction)

def _alloc_worldconstruction_buffer_callback(ptr, count):
    return util._allocate_array(ptr, WorldConstruction, count)

_worldconstruction_functype = CFUNCTYPE(c_int, POINTER(_worldconstruction_ptr), c_uint)
_worldconstruction_func = _worldconstruction_functype(_alloc_worldconstruction_buffer_callback)
_register_callback("alloc_worldconstruction_buffer_callback", _worldconstruction_func)

class MapBlock40d(Structure):
    _fields_ = [("tiletypes", TileTypes40d),
                ("designation", Designations40d),
                ("occupancy", Occupancies40d),
                ("biome_indices", BiomeIndices40d),
                ("origin", c_uint),
                ("blockflags", BlockFlags),
                ("global_feature", c_short),
                ("local_feature", c_short)]
                

class ViewScreen(Structure):
    _fields_ = [("type", c_int)]

class Matgloss(Structure):
    _fields_ = [("id", _dfhack_string),
                ("fore", c_byte),
                ("back", c_byte),
                ("bright", c_byte),
                ("name", _dfhack_string)]

_matgloss_ptr = POINTER(Matgloss)

def _alloc_matgloss_buffer_callback(ptr, count):
    return util._allocate_array(ptr, Matgloss, count)

_matgloss_functype = CFUNCTYPE(c_int, POINTER(_matgloss_ptr), c_uint)
_matgloss_func = _matgloss_functype(_alloc_matgloss_buffer_callback)
_register_callback("alloc_matgloss_buffer_callback", _matgloss_func)

class MatglossPair(Structure):
    _fields_ = [("type", c_short),
                ("index", c_int)]

class DescriptorColor(Structure):
    _fields_ = [("id", _dfhack_string),
                ("r", c_float),
                ("v", c_float),
                ("b", c_float),
                ("name", _dfhack_string)]

def _alloc_descriptor_buffer_callback(ptr, count):
    return util._allocate_array(ptr, DescriptorColor, count)

_descriptor_functype = CFUNCTYPE(c_int, POINTER(DescriptorColor), c_uint)
_descriptor_func = _descriptor_functype(_alloc_descriptor_buffer_callback)
_register_callback("alloc_descriptor_buffer_callback", _descriptor_func)

class MatglossPlant(Structure):
    _fields_ = [("id", _dfhack_string),
                ("fore", c_ubyte),
                ("back", c_ubyte),
                ("bright", c_ubyte),
                ("name", _dfhack_string),
                ("drink_name", _dfhack_string),
                ("food_name", _dfhack_string),
                ("extract_name", _dfhack_string)]

class MatglossOther(Structure):
    _fields_ = [("rawname", c_char * 128)]

def _alloc_matgloss_other_buffer_callback(count):
    return util._allocate_array(ptr, MatglossOther, count)

_matgloss_other_functype = CFUNCTYPE(c_int, POINTER(MatglossOther), c_uint)
_matgloss_other_func = _matgloss_other_functype(_alloc_matgloss_other_buffer_callback)
_register_callback("alloc_matgloss_other_buffer_callback", _matgloss_other_func)

class Building(Structure):
    _fields_ = [("origin", c_uint),
                ("vtable", c_uint),
                ("x1", c_uint),
                ("y1", c_uint),
                ("x2", c_uint),
                ("y2", c_uint),
                ("z", c_uint),
                ("material", MatglossPair),
                ("type", c_uint)]

class CustomWorkshop(Structure):
    _fields_ = [("index", c_uint),
                ("name", c_char * 256)]

def _alloc_custom_workshop_buffer_callback(count):
    return util._allocate_array(ptr, CustomWorkshop, count)

_custom_workshop_functype = CFUNCTYPE(c_int, POINTER(CustomWorkshop), c_uint)
_custom_workshop_func = _custom_workshop_functype(_alloc_custom_workshop_buffer_callback)
_register_callback("alloc_t_customWorkshop_buffer_callback", _custom_workshop_func)

class Construction(Structure):
    _fields_ = [("x", c_ushort),
                ("y", c_ushort),
                ("z", c_ushort),
                ("form", c_ushort),
                ("unk_8", c_ushort),
                ("mat_type", c_ushort),
                ("mat_idx", c_ushort),
                ("unk3", c_ushort),
                ("unk4", c_ushort),
                ("unk5", c_ushort),
                ("unk6", c_uint),
                ("origin", c_uint)]

class Tree(Structure):
    _fields_ = [("type", c_ushort),
                ("material", c_ushort),
                ("x", c_ushort),
                ("y", c_ushort),
                ("z", c_ushort),
                ("address", c_uint)]

class Material(Structure):
    _fields_ = [("itemType", c_short),
                ("subType", c_short),
                ("subIndex", c_short),
                ("index", c_int),
                ("flags", c_uint)]

def _alloc_material_buffer_callback(count):
    return util._allocate_array(ptr, Material, count)

_material_functype = CFUNCTYPE(c_int, POINTER(Material), c_uint)
_material_func = _material_functype(_alloc_material_buffer_callback)
_register_callback("alloc_t_material_buffer_callback", _material_func)

class Skill(Structure):
    _fields_ = [("id", c_uint),
                ("experience", c_uint),
                ("rating", c_uint)]

class Job(Structure):
    _fields_ = [("active", c_byte),
                ("jobId", c_uint),
                ("jobType", c_ubyte),
                ("occupationPtr", c_uint)]

class Like(Structure):
    _fields_ = [("type", c_short),
                ("itemClass", c_short),
                ("itemIndex", c_short),
                ("material", MatglossPair),
                ("active", c_byte)]

class Attribute(Structure):
    _fields_ = [("level", c_uint),
                ("field_4", c_uint),
                ("field_8", c_uint),
                ("field_C", c_uint),
                ("leveldiff", c_uint),
                ("field_14", c_uint),
                ("field_18", c_uint)]

class Name(Structure):
    _fields_ = [("first_name", _dfhack_string),
                ("nickname", _dfhack_string),
                ("words", (c_int * 7)),
                ("parts_of_speech", (c_ushort * 7)),
                ("language", c_uint),
                ("has_name", c_byte)]

class Note(Structure):
    _fields_ = [("symbol", c_char),
                ("foreground", c_ushort),
                ("background", c_ushort),
                ("name", _dfhack_string),
                ("x", c_ushort),
                ("y", c_ushort),
                ("z", c_ushort)]

class Settlement(Structure):
    _fields_ = [("origin", c_uint),
                ("name", Name),
                ("world_x", c_short),
                ("world_y", c_short),
                ("local_x1", c_short),
                ("local_x2", c_short),
                ("local_y1", c_short),
                ("local_y2", c_short)]

_NUM_CREATURE_TRAITS = 30
_NUM_CREATURE_LABORS = 102

class Soul(Structure):
    _fields_ = [("numSkills", c_ubyte),
                ("skills", (Skill * 256)),
                ("traits", (c_ushort * _NUM_CREATURE_TRAITS)),
                ("analytical_ability", Attribute),
                ("focus", Attribute),
                ("willpower", Attribute),
                ("creativity", Attribute),
                ("intuition", Attribute),
                ("patience", Attribute),
                ("memory", Attribute),
                ("linguistic_ability", Attribute),
                ("spatial_sense", Attribute),
                ("musicality", Attribute),
                ("kinesthetic_sense", Attribute),
                ("empathy", Attribute),
                ("social_awareness", Attribute)]

_MAX_COLORS = 15

class Creature(Structure):
    _fields_ = [("origin", c_uint),
                ("x", c_ushort),
                ("y", c_ushort),
                ("z", c_ushort),
                ("race", c_uint),
                ("civ", c_int),
                ("flags1", CreatureFlags1),
                ("flags2", CreatureFlags2),
                ("name", Name),
                ("mood", c_short),
                ("mood_skill", c_short),
                ("artifact_name", Name),
                ("profession", c_ubyte),
                ("custom_profession", _dfhack_string),
                ("labors", (c_ubyte * _NUM_CREATURE_LABORS)),
                ("current_job", Job),
                ("happiness", c_uint),
                ("id", c_uint),
                ("strength", Attribute),
                ("agility", Attribute),
                ("toughness", Attribute),
                ("endurance", Attribute),
                ("recuperation", Attribute),
                ("disease_resistance", Attribute),
                ("squad_leader_id", c_int),
                ("sex", c_ubyte),
                ("caste", c_ushort),
                ("pregnancy_timer", c_uint),
                ("has_default_soul", c_byte),
                ("defaultSoul", Soul),
                ("nbcolors", c_uint),
                ("color", (c_uint * _MAX_COLORS)),
                ("birth_year", c_int),
                ("birth_time", c_uint)]

class CreatureExtract(Structure):
    _fields_ = [("rawname", _dfhack_string)]

class BodyPart(Structure):
    _fields_ = [("id", _dfhack_string),
                ("category", _dfhack_string),
                ("single", _dfhack_string),
                ("plural", _dfhack_string)]

class ColorModifier(Structure):
    _fields_ = [("part", _dfhack_string),
                ("colorlist", POINTER(c_uint)),
                ("colorlistLength", c_uint),
                ("startdate", c_uint),
                ("enddate", c_uint)]

    def __init__(self):
        self.colorlistLength = 0

_colormodifier_ptr = POINTER(ColorModifier)

class CreatureCaste(Structure):
    _fields_ = [("rawname", _dfhack_string),
                ("singular", _dfhack_string),
                ("plural", _dfhack_string),
                ("adjective", _dfhack_string),
                ("color_modifier", _colormodifier_ptr),
                ("color_modifier_length", c_uint),
                ("bodypart", POINTER(BodyPart)),
                ("bodypart_length", c_uint),
                ("strength", Attribute),
                ("agility", Attribute),
                ("toughness", Attribute),
                ("endurance", Attribute),
                ("recuperation", Attribute),
                ("disease_resistance", Attribute),
                ("analytical_ability", Attribute),
                ("focus", Attribute),
                ("willpower", Attribute),
                ("creativity", Attribute),
                ("intuition", Attribute),
                ("patience", Attribute),
                ("memory", Attribute),
                ("linguistic_ability", Attribute),
                ("spatial_sense", Attribute),
                ("musicality", Attribute),
                ("kinesthetic_sense", Attribute)]

_creaturecaste_ptr = POINTER(CreatureCaste)

class TileColor(Structure):
    _fields_ = [("fore", c_ushort),
                ("back", c_ushort),
                ("bright", c_ushort)]

class ColorDescriptor(Structure):
    _fields_ = [("colorlistLength", c_uint)]

class CasteDescriptor(Structure):
    _fields_ = [("color_descriptors", POINTER(ColorDescriptor)),
                ("colorModifierLength", c_uint),
                ("bodypartLength", c_uint)]

class CreatureTypeDescriptor(Structure):
    _fields_ = [("caste_descriptors", POINTER(CasteDescriptor)),
                ("castesCount", c_uint),
                ("extractCount", c_uint)]

class CreatureType(Structure):
    _fields_ = [("rawname", _dfhack_string),
                ("castes", _creaturecaste_ptr),
                ("castes_count", c_uint),
                ("extract", POINTER(CreatureExtract)),
                ("extract_count", c_uint),
                ("tile_character", c_ubyte),
                ("tilecolor", TileColor)]

_creaturetype_ptr = POINTER(CreatureType)

def _alloc_creaturetype_buffer(ptr, descriptors, count):
    arr_list = []
    c_arr = (CreatureType * count)()
    
    for i, v in enumerate(c_arr):
        v.castesCount = descriptors[i].castesCount
        v_caste_arr = (CreatureCaste * v.castesCount)()
        
        for j, v_c in enumerate(v_caste_arr):
            caste_descriptor = descriptors[i].caste_descriptors[j]
            
            v_c.colorModifierLength = caste_descriptor.colorModifierLength
            c_color_arr = (ColorModifier * v_c.colorModifierLength)()
            
            for k, c_c in enumerate(c_color_arr):
                color_descriptor = caste_descriptor.color_descriptors[k]
                
                c_c.colorlistLength = color_descriptor.colorlistLength
                
                c_color_list_arr = (c_uint * c_c.colorlistLength)()
                
                p = cast(c_color_list_arr, POINTER(c_uint))
                
                c_c.colorlist = p
                
                arr_list.extend((c_color_list_arr, p))
            
            c_p = cast(c_color_arr, POINTER(ColorModifier))
            v_c.colorModifier = c_p
            
            v_c.bodypartLength = caste_descriptor.bodypartLength
            c_bodypart_arr = (BodyPart * v_c.bodypartLength)()
            
            b_p = cast(c_bodypart_arr, POINTER(BodyPart))
            v_c.bodypart = b_p
            
            arr_list.extend((c_color_arr, c_p, c_bodypart_arr, b_p))
        
        v.extractCount = descriptors[i].extractCount
        v_extract_arr = (CreatureExtract * v.extractCount)()
        
        caste_p = cast(v_caste_arr, POINTER(CreatureCaste))        
        v.castes = caste_p
        
        extract_p = cast(v_extract_arr, POINTER(CreatureExtract))
        v.extract = extract_p
        
        arr_list.extend((v_caste_arr, caste_p, v_extract_arr, extract_p))
    
    p = cast(c_arr, _creaturetype_ptr)
    ptr[0] = p
    print ""
    
    pointer_dict[addressof(c_arr)] = (ptr, c_arr, p, arr_list)
    
    return 1

_alloc_creaturetype_buffer_functype = CFUNCTYPE(c_int, POINTER(_creaturetype_ptr), POINTER(CreatureTypeDescriptor), c_uint)
_alloc_creaturetype_buffer_func = _alloc_creaturetype_buffer_functype(_alloc_creaturetype_buffer)
_register_callback("alloc_creaturetype_buffer_callback", _alloc_creaturetype_buffer_func)

class GameModes(Structure):
    _fields_ = [("control_mode", c_ubyte),
                ("game_mode", c_ubyte)]
