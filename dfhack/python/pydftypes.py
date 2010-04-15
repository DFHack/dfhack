from collections import namedtuple

Note = namedtuple("Note", "symbol, foreground, background, name, position")
Construction = namedtuple("Construction", "position, form, unk_8, mat_type, mat_idx, unk3, unk4, unk5, unk6, origin")

class Name(object):
    __slots__ = ["first_name", "nickname", "language", "has_name", "words", "parts_of_speech"]

class Soul(object):
    pass