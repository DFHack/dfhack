from collections import namedtuple

Position2D = namedtuple("Position2D", "x, y")
Position3D = namedtuple("Position3D", "x, y, z")
Rectangle = namedtuple("Rectangle", "x1, y1, x2, y2")
Note = namedtuple("Note", "symbol, foreground, background, name, position")
Construction = namedtuple("Construction", "position, form, unk_8, mat_type, mat_idx, unk3, unk4, unk5, unk6, origin")
Vein = namedtuple("Vein", "vtable, type, flags, address, assignment")
FrozenLiquidVein = namedtuple("FrozenLiquidVein", "vtable, address, tiles")
SpatterVein = namedtuple("SpatterVein", "vtable, address, mat1, unk1, mat2, mat3, intensity")
Settlement = namedtuple("Settlement", "origin, name, world_pos, local_pos")
Attribute = namedtuple("Attribute", "level, field_4, field_8, field_C, leveldiff, field_14, field_18");
Skill = namedtuple("Skill", "id, experience, rating")
Tree = namedtuple("Tree", "type, material, position, address")
CreatureCaste = namedtuple("CreatureCaste", "rawname, singular, plural, adjective")
Matgloss = namedtuple("Matgloss", "id, fore, back, bright, name")
DescriptorColor = namedtuple("DescriptorColor", "id, r, v, b, name")
CreatureTypeEx = namedtuple("CreatureTypeEx", "rawname, castes, tile_character, tilecolor")
TileColor = namedtuple("TileColor", "fore, back, bright")

class Name(object):
    __slots__ = ["first_name", "nickname", "language", "has_name", "words", "parts_of_speech"]

class Soul(object):
    def __init__(self, *args, **kwds):
        if kwds:
            for k, v in kwds.iteritems():
                self.__dict__[k] = v

class MapBlock40d(object):
    pass