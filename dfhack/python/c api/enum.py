#found this in the cookbook:  http://code.activestate.com/recipes/576415/

from ctypes import c_uint

class C_EnumerationType(type(c_uint)):
    def __new__(metacls, name, bases, dictionary):
        if not "_members_" in dictionary:
            _members_ = {}

            for key, value in dictionary.iteritems():
                if not key.startswith("_"):
                    _members_[key] = value

            dictionary["_members_"] = _members_

        cls = type(c_uint).__new__(metacls, name, bases, dictionary)

        for key, value in cls._members_.iteritems():
            globals()[key] = value

        return cls

    def __contains__(self, value):
        return value in self._members_.values()

    def __repr__(self):
        return "<Enumeration %s>" % self.__name__

class C_Enumeration(c_uint):
    __metaclass__ = C_EnumerationType
    _members_ = {}

    def __init__(self, value):
        for key, value in self._members_.iteritems():
            if v == value:
                self.name = key
                break
        else:
            raise ValueError("No enumeration member with value %r" % value)

        c_uint.__init__(self, value)

    def __repr__(self):
        return "<member %s=%d of %r>" % (self.name, self.value, self.__class__)

    @classmethod
    def from_param(cls, param):
        if isinstance(param, C_Enumeration):
            if param.__class__ != cls:
                raise ValueError("Cannot mix enumeration members")
            else:
                return param
        else:
            return cls(param)

FeatureType = C_EnumerationType("FeatureType",
                                (c_uint,),
                                {"Other" : 0,
                                 "Adamantine_Tube" : 1,
                                 "Underworld" : 2,
                                 "Hell_Temple" : 3})

BiomeOffset = C_EnumerationType("BiomeOffset",
                                (c_uint,),
                                {"NorthWest" : 0,
                                 "North" : 1,
                                 "NorthEast" : 2,
                                 "West" : 3,
                                 "Here" : 4,
                                 "East" : 5,
                                 "SouthWest" : 6,
                                 "South" : 7,
                                 "SouthEast" : 8,
                                 "BiomeCount" : 9})

TrafficType = C_EnumerationType("TrafficType",
                                (c_uint,),
                                {"Normal" : 0,
                                 "Low" : 1,
                                 "High" : 2,
                                 "Restricted" : 3})

DesignationType = C_EnumerationType("DesignationType",
                                    (c_uint,),
                                    {"No" : 0,
                                     "Default" : 1,
                                     "UD_Stair" : 2,
                                     "Channel" : 3,
                                     "Ramp" : 4,
                                     "D_Stair" : 5,
                                     "U_Stair" : 6,
                                     "Whatever" : 7})

LiquidType = C_EnumerationType("LiquidType",
                               (c_uint,),
                               {"Water" : 0,
                                "Magma" : 1})
