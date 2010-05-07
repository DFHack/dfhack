from ctypes import *
from collections import namedtuple
from pydfhackflags import *
from enum import *

Position3D = namedtuple("Position3D", "x, y, z")
Rectangle = namedtuple("Rectangle", "x1, y1, x2, y2")
Note = namedtuple("Note", "symbol, foreground, background, name, position")
Settlement = namedtuple("Settlement", "origin, name, world_pos, local_pos")
Attribute = namedtuple("Attribute", "level, field_4, field_8, field_C, leveldiff, field_14, field_18");
Skill = namedtuple("Skill", "id, experience, rating")
Tree = namedtuple("Tree", "type, material, position, address")
CreatureCaste = namedtuple("CreatureCaste", "rawname, singular, plural, adjective")
CreatureTypeEx = namedtuple("CreatureTypeEx", "rawname, castes, tile_character, tilecolor")
TileColor = namedtuple("TileColor", "fore, back, bright")
Name = namedtuple("Name", "first_name, nickname, language, has_name, words, parts_of_speech")

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

class Soul(object):
    def __init__(self, *args, **kwds):
        if kwds:
            for k, v in kwds.iteritems():
                self.__dict__[k] = v

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

class MatglossPair(Structure):
    _fields_ = [("type", c_short),
                ("index", c_int)]

class Descriptor_Color(Structure):
    _fields_ = [("id", c_char * 128),
                ("r", c_float),
                ("v", c_float),
                ("b", c_float),
                ("name", c_char * 128)]

class MatglossOther(Structure):
    _fields_ = [("rawname", c_char * 128)]

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
