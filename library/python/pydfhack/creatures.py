from ctypes import *
from dftypes import libdfhack, Creature, Material
import util

libdfhack.Creatures_WriteLabors.argtypes = [ c_void_p, c_uint, POINTER(c_ubyte) ]

class Creatures(object):
    def __init__(self, ptr):
        print ptr
        self._c_ptr = ptr

        self._d_race_index = None
        self._d_civ_id = None

    def start(self):
        n = c_uint(0)

        if libdfhack.Creatures_Start(self._c_ptr, byref(n)) > 0:
            return int(n.value)
        else:
            return -1

    def finish(self):
        return libdfhack.Creatures_Finish(self._c_ptr) > 0

    def read_creature(self, index):
        c = Creature()

        if libdfhack.Creatures_ReadCreature(self._c_ptr, c_int(index), byref(c)) > 0:
            return c
        else:
            return None

    def read_creature_in_box(self, index, pos1, pos2):
        c = Creature()

        x1, y1, z1 = c_uint(pos1[0]), c_uint(pos1[1]), c_uint(pos1[2])
        x2, y2, z2 = c_uint(pos2[0]), c_uint(pos2[1]), c_uint(pos2[2])

        retval = libdfhack.Creatures_ReadCreatureInBox(self._c_ptr, byref(c), x1, y1, z1, x2, y2, z2)

        return (retval, c)

    def write_labors(self, index, labors):
        return libdfhack.Creatures_WriteLabors(self._c_ptr, c_uint(index), labors) > 0

    def read_job(self, creature):
        return libdfhack.Creatures_ReadJob(self._c_ptr, byref(creature))

    @property
    def dwarf_race_index(self):
        if self._d_race_index is None:
            self._d_race_index =libdfhack.Creatures_GetDwarfRaceIndex(self._c_ptr)

        return self._d_race_index

    @property
    def dwarf_civ_id(self):
        if self._d_civ_id is None:
            self._d_civ_id = libdfhack.Creatures_GetDwarfCivId(self._c_ptr)

        return self._d_civ_id
