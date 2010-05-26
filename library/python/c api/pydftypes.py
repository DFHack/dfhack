from ctypes import *
from pydfhackflags import *
from enum import *
from util import *

libdfhack = cdll.libdfhack

libdfhack.alloc_byte_buffer_callback = alloc_byte_buffer
libdfhack.alloc_ubyte_buffer_callback = alloc_ubyte_buffer
libdfhack.alloc_short_buffer_callback = alloc_short_buffer
libdfhack.alloc_ushort_buffer_callback = alloc_ushort_buffer
libdfhack.alloc_int_buffer_callback = alloc_int_buffer
libdfhack.alloc_uint_buffer_callback = alloc_uint_buffer
libdfhack.alloc_char_buffer_callback = alloc_char_buffer

int_ptr = POINTER(c_int)
uint_ptr = POINTER(c_uint)

_arr_create_func = CFUNCTYPE(c_void_p, c_int)

TileTypes40d = ((c_int * 16) * 16)
BiomeIndices40d = c_ubyte * 16
Temperatures = ((c_ushort * 16) * 16)
Designations40d = ((DesignationFlags * 16) * 16)
Occupancies40d = ((OccupancyFlags * 16) * 16)

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

class FrozenLiquidVein(Structure):
    _fields_ = [("vtable", c_uint),
                ("tiles", TileTypes40d),
                ("address_of", c_uint)]

class SpatterVein(Structure):
    _fields_ = [("vtable", c_uint),
                ("mat1", c_ushort),
                ("unk1", c_ushort),
                ("mat2", c_uint),
                ("mat3", c_ushort),
                ("intensity", ((c_ubyte * 16) * 16)),
                ("address_of", c_uint)]

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
    _fields_ = [("id", c_char * 128),
                ("fore", c_byte),
                ("back", c_byte),
                ("bright", c_byte),
                ("name", c_char * 128)]

def _alloc_matgloss_buffer_callback(count):
    allocated = _allocate_array(Matgloss, count)

    return allocated[1]

libdfhack.alloc_matgloss_buffer_callback = CFUNCTYPE(POINTER(Matgloss), c_int)(_alloc_matgloss_buffer_callback)

class MatglossPair(Structure):
    _fields_ = [("type", c_short),
                ("index", c_int)]

class DescriptorColor(Structure):
    _fields_ = [("id", c_char * 128),
                ("r", c_float),
                ("v", c_float),
                ("b", c_float),
                ("name", c_char * 128)]

def _alloc_descriptor_buffer_callback(count):
    allocated = _allocate_array(DescriptorColor, count)

    return allocated[1]

libdfhack.alloc_descriptor_buffer_callback = CFUNCTYPE(POINTER(DescriptorColor), c_int)(_alloc_descriptor_buffer_callback)

class MatglossOther(Structure):
    _fields_ = [("rawname", c_char * 128)]

def _alloc_matgloss_other_buffer_callback(count):
    allocated = _allocate_array(MatglossOther, count)

    return allocated[1]

libdfhack.alloc_matgloss_other_buffer_callback = CFUNCTYPE(POINTER(MatglossOther), c_int)(_alloc_matgloss_other_buffer_callback)

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

class Skill(Structure):
    _fields_ = [("id", c_ushort),
                ("experience", c_uint),
                ("rating", c_ushort)]

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
    _fields_ = [("first_name", (c_char * 128)),
                ("nickname", (c_char * 128)),
                ("words", (c_int * 7)),
                ("parts_of_speech", (c_ushort * 7)),
                ("language", c_uint),
                ("has_name", c_byte)]

class Note(Structure):
    _fields_ = [("symbol", c_char),
                ("foreground", c_ushort),
                ("background", c_ushort),
                ("name", (c_char * 128)),
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
                ("custom_profession", (c_char * 128)),
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
                ("color", (c_uint * _MAX_COLORS))]

class CreatureExtract(Structure):
    _fields_ = [("rawname", (c_char * 128))]

class BodyPart(Structure):
    _fields_ = [("id", (c_char * 128)),
                ("category", (c_char * 128)),
                ("single", (c_char * 128)),
                ("plural", (c_char * 128))]

class ColorModifier(Structure):
    _fields_ = [("part", (c_char * 128)),
                ("colorlist", POINTER(c_uint)),
                ("colorlistLength", c_uint)]

    def __init__(self):
        self.part[0] = '\0'
        self.colorlistLength = 0

ColorModifierPtr = POINTER(ColorModifier)

def _alloc_empty_colormodifier_callback():
    return ColorModifierPtr(ColorModifier())

libdfhack.alloc_empty_colormodifier_callback = CFUNCTYPE(ColorModifierPtr)(_alloc_empty_colormodifier_callback)
