from collections import namedtuple

Note = namedtuple("Note", "symbol, foreground, background, name, position")
Construction = namedtuple("Construction", "position, form, unk_8, mat_type, mat_idx, unk3, unk4, unk5, unk6, origin")
Vein = namedtuple("Vein", "vtable, type, flags, address, assignment")
FrozenLiquidVein = namedtuple("FrozenLiquidVein", "vtable, address, tiles")
SpatterVein = namedtuple("SpatterVein", "vtable, address, mat1, unk1, mat2, mat3, intensity")

class Name(object):
    __slots__ = ["first_name", "nickname", "language", "has_name", "words", "parts_of_speech"]

class Soul(object):
    pass

class MapBlock40d(object):
    pass