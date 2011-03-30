# -*- coding: utf-8 -*-
from ctypes import Structure, Union, c_uint
from enum import *

class DesignationStruct(Structure):
    _fields_ = [("flow_size", c_uint, 3),
                ("pile", c_uint, 1),
                ("dig", DesignationType, 3),
                ("smooth", c_uint, 2),
                ("hidden", c_uint, 1),
                ("geolayer_index", c_uint, 4),
                ("light", c_uint, 1),
                ("subterranean", c_uint, 1),
                ("skyview", c_uint, 1),
                ("biome", c_uint, 4),
                ("liquid_type", c_uint, 1),
                ("water_table", c_uint, 1),
                ("rained", c_uint, 1),
                ("traffic", c_uint, 2),
                ("flow_forbid", c_uint, 1),
                ("liquid_static", c_uint, 1),
                ("feature_local", c_uint, 1),
                ("feature_global", c_uint, 1),
                ("water_stagnant", c_uint, 1),
                ("water_salt", c_uint, 1)]

class DesignationFlags(Union):
    _fields_ = [("whole", c_uint, 32),
                ("bits", DesignationStruct)]
    
    def __init__(self, initial = 0):
            self.whole = initial
    
    def __int__(self):
        return self.whole

class OccupancyStruct(Structure):
    _fields_ = [("building", c_uint, 3),
                ("unit", c_uint, 1),
                ("unit_grounded", c_uint, 1),
                ("item", c_uint, 1),
                ("splatter", c_uint, 26)]

class OccupancyFlags(Union):
    _fields_ = [("whole", c_uint, 32),
                ("bits", OccupancyStruct)]

    def __init__(self, initial = 0):
        self.whole = initial
    
    def __int__(self):
        return self.whole

class CreatureStruct1(Structure):
    _fields_ = [("move_state", c_uint, 1),
                ("dead", c_uint, 1),
                ("has_mood", c_uint, 1),
                ("had_mood", c_uint, 1),
                ("marauder", c_uint, 1),
                ("drowning", c_uint, 1),
                ("merchant", c_uint, 1),
                ("forest", c_uint, 1),
                ("left", c_uint, 1),
                ("rider", c_uint, 1),
                ("incoming", c_uint, 1),
                ("diplomat", c_uint, 1),
                ("zombie", c_uint, 1),
                ("skeleton", c_uint, 1),
                ("can_swap", c_uint, 1),
                ("on_ground", c_uint, 1),
                ("projectile", c_uint, 1),
                ("active_invader", c_uint, 1),
                ("hidden_in_ambush", c_uint, 1),
                ("invader_origin", c_uint, 1),
                ("coward", c_uint, 1),
                ("hidden_ambusher", c_uint, 1),
                ("invades", c_uint, 1),
                ("check_flows", c_uint, 1),
                ("ridden", c_uint, 1),
                ("caged", c_uint, 1),
                ("tame", c_uint, 1),
                ("chained", c_uint, 1),
                ("royal_guard", c_uint, 1),
                ("fortress_guard", c_uint, 1),
                ("suppress_wield", c_uint, 1),
                ("important_historical_figure", c_uint, 1)]

class CreatureFlags1(Union):
    _fields_ = [("whole", c_uint, 32),
                ("bits", CreatureStruct1)]

    def __init__(self, initial = 0):
        self.whole = initial
    
    def __int__(self):
        return self.whole

class CreatureStruct2(Structure):
    _fields_ = [("swimming", c_uint, 1),
                ("sparring", c_uint, 1),
                ("no_notify", c_uint, 1),
                ("unused", c_uint, 1),
                ("calculated_nerves", c_uint, 1),
                ("calculated_bodyparts", c_uint, 1),
                ("important_historical_figure", c_uint, 1),
                ("killed", c_uint, 1),
                ("cleanup_1", c_uint, 1),
                ("cleanup_2", c_uint, 1),
                ("cleanup_3", c_uint, 1),
                ("for_trade", c_uint, 1),
                ("trade_resolved", c_uint, 1),
                ("has_breaks", c_uint, 1),
                ("gutted", c_uint, 1),
                ("circulatory_spray", c_uint, 1),
                ("locked_in_for_trading", c_uint, 1),
                ("slaughter", c_uint, 1),
                ("underworld", c_uint, 1),
                ("resident", c_uint, 1),
                ("cleanup_4", c_uint, 1),
                ("calculated_insulation", c_uint, 1),
                ("visitor_uninvited", c_uint, 1),
                ("visitor", c_uint, 1),
                ("calculated_inventory", c_uint, 1),
                ("vision_good", c_uint, 1),
                ("vision_damaged", c_uint, 1),
                ("vision_missing", c_uint, 1),
                ("breathing_good", c_uint, 1),
                ("breathing_problem", c_uint, 1),
                ("roaming_wilderness_population_source", c_uint, 1),
                ("roaming_wilderness_population_source_not_a_map_feature", c_uint, 1)]

class CreatureFlags2(Union):
    _fields_ = [("whole", c_uint, 32),
                ("bits", CreatureStruct2)]

    def __init__(self, initial = 0):
        self.whole = initial
    
    def __int__(self):
        return self.whole

class ItemStruct(Structure):
    _fields_ = [("on_ground", c_uint, 1),
                ("in_job", c_uint, 1),
                ("in_inventory", c_uint, 1),
                ("u_ngrd1", c_uint, 1),
                ("in_workshop", c_uint, 1),
                ("u_ngrd2", c_uint, 1),
                ("u_ngrd3", c_uint, 1),
                ("rotten", c_uint, 1),
                ("unk1", c_uint, 1),
                ("u_ngrd4", c_uint, 1),
                ("unk2", c_uint, 1),
                ("u_ngrd5", c_uint, 1),
                ("unk3", c_uint, 1),
                ("u_ngrd6", c_uint, 1),
                ("narrow", c_uint, 1),
                ("u_ngrd7", c_uint, 1),
                ("worn", c_uint, 1),
                ("unk4", c_uint, 1),
                ("u_ngrd8", c_uint, 1),
                ("forbid", c_uint, 1),
                ("unk5", c_uint, 1),
                ("dump", c_uint, 1),
                ("on_fire", c_uint, 1),
                ("melt", c_uint, 1),
                ("hidden", c_uint, 1),
                ("u_ngrd9", c_uint, 1),
                ("unk6", c_uint, 1),
                ("unk7", c_uint, 1),
                ("unk8", c_uint, 1),
                ("unk9", c_uint, 1),
                ("unk10", c_uint, 1),
                ("unk11", c_uint, 1)]

class ItemFlags(Union):
    _fields_ = [("whole", c_uint, 32),
                ("bits", ItemStruct)]

    def __init__(self, initial = 0):
        self.whole = initial
    
    def __int__(self):
        return self.whole

class BlockFlagStruct(Structure):
    _fields_ = [("designated", c_uint, 1),
                ("unk_1", c_uint, 1),
                ("liquid_1", c_uint, 1),
                ("liquid_2", c_uint, 1),
                ("unk_2", c_uint, 28)]

class BlockFlags(Union):
    _fields_ = [("whole", c_uint, 32),
                ("bits", BlockFlagStruct)]
    
    def __init__(self, inital = 0):
        self.whole = initial
    
    def __int__(self):
        return self.whole
