from ctypes import *
from dftypes import *
from util import _uintify, int_ptr, uint_ptr, check_pointer_cache

_MAX_DIM = 0x300
_MAX_DIM_SQR = _MAX_DIM * _MAX_DIM

libdfhack.Maps_getSize.argtypes = [ c_void_p, uint_ptr, uint_ptr, uint_ptr ]
libdfhack.Maps_getPosition.argtypes = [ c_void_p, int_ptr, int_ptr, int_ptr ]

libdfhack.Maps_ReadTileTypes.argtypes = [ c_void_p, c_uint, c_uint, c_uint, POINTER(TileTypes40d) ]
libdfhack.Maps_WriteTileTypes.argtypes = [ c_void_p, c_uint, c_uint, c_uint, POINTER(TileTypes40d) ]
libdfhack.Maps_ReadDesignations.argtypes = [ c_void_p, c_uint, c_uint, c_uint, POINTER(Designations40d) ]
libdfhack.Maps_WriteDesignations.argtypes = [ c_void_p, c_uint, c_uint, c_uint, POINTER(Designations40d) ]
libdfhack.Maps_ReadTemperatures.argtypes = [ c_void_p, c_uint, c_uint, c_uint, POINTER(Temperatures) ]
libdfhack.Maps_WriteTemperatures.argtypes = [ c_void_p, c_uint, c_uint, c_uint, POINTER(Temperatures) ]
libdfhack.Maps_ReadOccupancy.argtypes = [ c_void_p, c_uint, c_uint, c_uint, POINTER(Occupancies40d) ]
libdfhack.Maps_WriteOccupancy.argtypes = [ c_void_p, c_uint, c_uint, c_uint, POINTER(Occupancies40d) ]
libdfhack.Maps_ReadRegionOffsets.argtypes = [ c_void_p, c_uint, c_uint, c_uint, POINTER(BiomeIndices40d) ]

libdfhack.Maps_ReadVegetation.restype = c_void_p

libdfhack.Maps_ReadStandardVeins.argtypes = [ c_void_p, c_uint, c_uint, c_uint ]
libdfhack.Maps_ReadFrozenVeins.argtypes = [ c_void_p, c_uint, c_uint, c_uint ]
libdfhack.Maps_ReadSpatterVeins.argtypes = [ c_void_p, c_uint, c_uint, c_uint ]
libdfhack.Maps_ReadGrassVeins.argtypes = [ c_void_p, c_uint, c_uint, c_uint ]
libdfhack.Maps_ReadWorldConstructions.argtypes = [ c_void_p, c_uint, c_uint, c_uint ]

libdfhack.Maps_ReadStandardVeins.restype = c_void_p
libdfhack.Maps_ReadFrozenVeins.restype = c_void_p
libdfhack.Maps_ReadSpatterVeins.restype = c_void_p
libdfhack.Maps_ReadGrassVeins.restype = c_void_p
libdfhack.Maps_ReadWorldConstructions.restype = c_void_p

libdfhack.Maps_ReadLocalFeatures.restype = c_void_p

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

        ux, uy, uz = _uintify(x, y, z)

        if libdfhack.Maps_ReadTileTypes(self._map_ptr, ux, uy, uz, tt) > 0:
            return tt
        else:
            return None

    def write_tile_types(self, x, y, z, tt):
        ux, uy, uz = _uintify(x, y, z)
        
        return libdfhack.Maps_WriteTileTypes(self._map_ptr,  ux, uy, uz, tt) > 0

    def read_designations(self, x, y, z):
        d = Designations40d()

        ux, uy, uz = _uintify(x, y, z)

        if libdfhack.Maps_ReadDesignations(self._map_ptr, ux, uy, uz, byref(d)) > 0:
            return d
        else:
            return None

    def write_designations(self, x, y, z, d):
        ux, uy, uz = _uintify(x, y, z)
        
        return libdfhack.Maps_WriteDesignations(self._map_ptr, ux, uy, uz, byref(d)) > 0

    def read_temperatures(self, x, y, z):
        t = Temperatures()

        ux, uy, uz = _uintify(x, y, z)

        if libdfhack.Maps_ReadDesignations(self._map_ptr, ux, uy, uz, t) > 0:
            return t
        else:
            return None

    def write_temperatures(self, x, y, z, t):
        ux, uy, uz = _uintify(x, y, z)
        
        return libdfhack.Maps_WriteDesignations(self._map_ptr, ux, uy, uz, t) > 0

    def read_occupancy(self, x, y, z):
        o = Occupancies40d()

        ux, uy, uz = _uintify(x, y, z)

        if libdfhack.Maps_ReadDesignations(self._map_ptr, ux, uy, uz, o) > 0:
            return o
        else:
            return None

    def write_occupancy(self, x, y, z, o):
        ux, uy, uz = _uintify(x, y, z)
        
        return libdfhack.Maps_WriteDesignations(self._map_ptr, ux, uy, uz, byref(o)) > 0

    def read_dirty_bit(self, x, y, z):
        bit = c_int(0)

        ux, uy, uz = _uintify(x, y, z)

        if libdfhack.Maps_ReadDirtyBit(self._map_ptr, ux, uy, uz, byref(bit)) > 0:
            if bit > 0:
                return True
            else:
                return False
        else:
            return None

    def write_dirty_bit(self, x, y, z, dirty):
        ux, uy, uz = _uintify(x, y, z)
        
        return libdfhack.Maps_WriteDirtyBit(self._map_ptr, ux, uy, uz, c_int(dirty)) > 0

    def read_features(self, x, y, z):
        lf = c_short()
        gf = c_short()

        ux, uy, uz = _uintify(x, y, z)

        libdfhack.Maps_ReadFeatures(self._map_ptr, ux, uy, uz, byref(lf), byref(fg))

        return (lf, gf)

    def write_local_feature(self, x, y, z, local_feature = -1):
        ux, uy, uz = _uintify(x, y, z)
        
        return libdfhack.Maps_WriteLocalFeature(self._map_ptr, ux, uy, uz, c_short(local_feature)) > 0

    def write_global_feature(self, x, y, z, global_feature = -1):
        ux, uy, uz = _uintify(x, y, z)
        
        return libdfhack.Maps_WriteGlobalFeature(self._map_ptr, ux, uy, uz, c_short(global_feature)) > 0

    def read_block_flags(self, x, y, z):
        bf = BlockFlags()

        ux, uy, uz = _uintify(x, y, z)

        if libdfhack.Maps_ReadBlockFlags(self._map_ptr, ux, uy, uz, byref(bf)) > 0:
            return bf
        else:
            return None

    def write_block_flags(self, x, y, z, block_flags):
        ux, uy, uz = _uintify(x, y, z)
        
        return libdfhack.Maps_WriteBlockFlags(self._map_ptr, ux, uy, uz, block_flags) > 0

    def read_region_offsets(self, x, y, z):
        bi = BiomeIndices40d()

        ux, uy, uz = _uintify(x, y, z)

        if libdfhack.Maps_ReadRegionOffsets(self._map_ptr, ux, uy, uz, byref(bi)) > 0:
            return bi
        else:
            return None
    
    def read_veins(self, x, y, z):
        ux, uy, uz = _uintify(x, y, z)
        
        return check_pointer_cache(libdfhack.Maps_ReadStandardVeins(self._map_ptr, ux, uy, uz))
    
    def read_frozen_veins(self, x, y, z):
        ux, uy, uz = _uintify(x, y, z)
        
        return check_pointer_cache(libdfhack.Maps_ReadFrozenVeins(self._map_ptr, ux, uy, uz))
    
    def read_spatter_veins(self, x, y, z):
        ux, uy, uz = _uintify(x, y, z)
        
        return check_pointer_cache(libdfhack.Maps_ReadSpatterVeins(self._map_ptr, ux, uy, uz))
    
    def read_grass_veins(self, x, y, z):
        ux, uy, uz = _uintify(x, y, z)
        
        return check_pointer_cache(libdfhack.Maps_ReadGrassVeins(self._map_ptr, ux, uy, uz))
    
    def read_world_constructions(self, x, y, z):
        ux, uy, uz = _uintify(x, y, z)
        
        return check_pointer_cache(libdfhack.Maps_ReadWorldConstructions(self._map_ptr, ux, uy, uz))
    
    def read_vegetation(self, x, y, z):
        ux, uy, uz = _uintify(x, y, z)
        
        return check_pointer_cache(libdfhack.Maps_ReadVegetation(self._map_ptr, ux, uy, uz))
    
    def read_local_features(self):
        f = libdfhack.Maps_ReadLocalFeatures(self._map_ptr)
        feature_dict = {}        
        f_arr = check_pointer_cache(f, False)
        
        if f_arr is not None:
            for node in f_arr:
                c = node.coordinate.xyz
                coord = MapPoint(c.x, c.y, c.z)
                f_list = [node.features[i] for i in xrange(node.feature_length)]
                feature_dict[coord] = f_list
        
        return feature_dict

    @property
    def size(self):
        x, y, z = (c_uint(0), c_uint(0), c_uint(0))

        retval = libdfhack.Maps_getSize(self._map_ptr, byref(x), byref(y), byref(z))

        return (int(x.value), int(y.value), int(z.value))
    
    @property
    def position(self):
        x, y, z = (c_int(0), c_int(0), c_int(0))
        
        libdfhack.Maps_getPosition(self._map_ptr, byref(x), byref(y), byref(z))
        
        return (int(x.value), int(y.value), int(z.value))

class MapPoint(object):
    __slots__ = ["_x", "_y", "_z", "_cmp_val"]
    
    def __init__(self, x = 0, y = 0, z = 0):
        self._x, self._y, self._z = x, y, z
        
        self._cmp_val = self._get_cmp_value()
    
    def _val_set(self, which, val):
        if which == 0:
            self._x = val
        elif which == 1:
            self._y = val
        elif which == 2:
            self._z = val
        
        self._cmp_val = self._get_cmp_value()
    
    x = property(fget = lambda self:  self._x, fset = lambda self, v:  self._val_set(0, v))
    y = property(fget = lambda self:  self._y, fset = lambda self, v:  self._val_set(1, v))
    z = property(fget = lambda self:  self._z, fset = lambda self, v:  self._val_set(2, v))
    
    def _get_cmp_value(self):
        return (self.z * _MAX_DIM_SQR) + (self.y * _MAX_DIM) + self.x
    
    #comparison operators
    def __eq__(self, other):
        return self.x == other.x and self.y == other.y and self.z == other.z
    
    def __ne__(self, other):
        return not self == other
    
    def __lt__(self, other):
        return self._cmp_val < other._cmp_val
    
    def __le__(self, other):
        return self < other or self == other
    
    def __gt__(self, other):
        return self._cmp_val > other._cmp_val
    
    def __ge__(self, other):
        return self > other or self == other
    
    #arithmetic operators    
    def __add__(self, num):
        return MapPoint(self.x, self.y, self.z + num)
    
    def __sub__(self, num):
        return MapPoint(self.x, self.y, self.z - num)
    
    def __div__(self, num):
        return MapPoint(self.x / num, self.y / num, self.z)
    
    def __mul__(self, num):
        return MapPoint(self.x * num, self.y * num, self.z)
    
    def __mod__(self, num):
        return MapPoint(self.x % num, self.y % num, self.z)