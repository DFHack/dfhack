# -*- coding: utf-8 -*-
from pydfhack import *

class DFAPI(API):
    def Read_Designations(self, x, y, z):
        d_list = [DesignationFlags(d) for d in API.Read_Designations(self, x, y, z)]

        return d_list
    def Read_Occupancy(self, x, y, z):
        o_list = [OccupancyFlags(o) for o in API.Read_Occupancy(self, x, y, z)]

        return o_list

class CreatureFlags1(_UnionBase):
    move_state = property(fget = lambda self: self._get_mask_bit(0),
                          fset = lambda self, val: self._set_mask_bit(0, val))
    dead = property(fget = lambda self: self._get_mask_bit(1),
                    fset = lambda self, val: self._set_mask_bit(1, val))
    has_mood = property(fget = lambda self: self._get_mask_bit(2),
                        fset = lambda self, val: self._set_mask_bit(2, val))
    had_mood = property(fget = lambda self: self._get_mask_bit(3),
                        fset = lambda self, val: self._set_mask_bit(3, val))
    
    marauder = property(fget = lambda self: self._get_mask_bit(4),
                        fset = lambda self, val: self._set_mask_bit(4, val))
    drowning = property(fget = lambda self: self._get_mask_bit(5),
                        fset = lambda self, val: self._set_mask_bit(5, val))
    merchant = property(fget = lambda self: self._get_mask_bit(6),
                        fset = lambda self, val: self._set_mask_bit(6, val))
    forest = property(fget = lambda self: self._get_mask_bit(7),
                      fset = lambda self, val: self._set_mask_bit(7, val))
    
    left = property(fget = lambda self: self._get_mask_bit(8),
                    fset = lambda self, val: self._set_mask_bit(8, val))
    rider = property(fget = lambda self: self._get_mask_bit(9),
                     fset = lambda self, val: self._set_mask_bit(9, val))
    incoming = property(fget = lambda self: self._get_mask_bit(10),
                        fset = lambda self, val: self._set_mask_bit(10, val))
    diplomat = property(fget = lambda self: self._get_mask_bit(11),
                        fset = lambda self, val: self._set_mask_bit(11, val))
    
    zombie = property(fget = lambda self: self._get_mask_bit(12),
                      fset = lambda self: self._set_mask_bit(12, val))
    skeleton = property(fget = lambda self: self._get_mask_bit(13),
                        fset = lambda self, val: self._set_mask_bit(13, val))
    can_swap = property(fget = lambda self: self._get_mask_bit(14),
                        fset = lambda self, val: self._set_mask_bit(14, val))
    on_ground = property(fget = lambda self: self._get_mask_bit(15),
                         fset = lambda self, val: self._set_mask_bit(15, val))
    
    projectile = property(fget = lambda self: self._get_mask_bit(16),
                          fset = lambda self, val: self._set_mask_bit(16, val))
    active_invader = property(fget = lambda self: self._get_mask_bit(17),
                              fset = lambda self, val: self._set_mask_bit(17, val))
    hidden_in_ambush = property(fget = lambda self: self._get_mask_bit(18),
                                fset = lambda self, val: self._set_mask_bit(18, val))
    invader_origin = property(fget = lambda self: self._get_mask_bit(19),
                              fset = lambda self, val: self._set_mask_bit(19, val))
    
    coward = property(fget = lambda self: self._get_mask_bit(20),
                      fset = lambda self, val: self._set_mask_bit(20, val))
    hidden_ambusher = property(fget = lambda self: self._get_mask_bit(21),
                               fset = lambda self, val: self._set_mask_bit(21, val))
    invades = property(fget = lambda self: self._get_mask_bit(22),
                       fset = lambda self, val: self._set_mask_bit(22, val))
    check_flows = property(fget = lambda self: self._get_mask_bit(23),
                           fset = lambda self, val: self._set_mask_bit(23, val))
    
    ridden = property(fget = lambda self: self._get_mask_bit(24),
                      fset = lambda self, val: self._set_mask_bit(24, val))
    caged = property(fget = lambda self: self._get_mask_bit(25),
                     fset = lambda self, val: self._set_mask_bit(25, val))
    tame = property(fget = lambda self: self._get_mask_bit(26),
                    fset = lambda self, val: self._set_mask_bit(26, val))
    chained = property(fget = lambda self: self._get_mask_bit(27),
                       fset = lambda self, val: self._set_mask_bit(27, val))
    
    royal_guard = property(fget = lambda self: self._get_mask_bit(28),
                           fset = lambda self, val: self._set_mask_bit(28, val))
    fortress_guard = property(fget = lambda self: self._get_mask_bit(29),
                              fset = lambda self, val: self._set_mask_bit(29, val))
    suppress_wield = property(fget = lambda self: self._get_mask_bit(30),
                              fset = lambda self, val: self._set_mask_bit(30, val))
    important_historial_figure = property(fget = lambda self: self._get_mask_bit(31),
                                          fset = lambda self, val: self._set_mask_bit(31, val))

class CreatureFlags2(_UnionBase):
    swimming = property(fget = lambda self: self._get_mask_bit(0),
                        fset = lambda self, val: self._set_mask_bit(0, val))
    sparring = property(fget = lambda self: self._get_mask_bit(1),
                        fset = lambda self, val: self._set_mask_bit(1, val))
    no_notify = property(fget = lambda self: self._get_mask_bit(2),
                         fset = lambda self, val: self._set_mask_bit(2, val))
    unused = property(fget = lambda self: self._get_mask_bit(3),
                      fset = lambda self, val: self._set_mask_bit(3, val))
    
    calculated_nerves = property(fget = lambda self: self._get_mask_bit(4),
                                 fset = lambda self, val: self._set_mask_bit(4, val))
    calculated_bodyparts = property(fget = lambda self: self._get_mask_bit(5),
                                    fset = lambda self, val: self._set_mask_bit(5, val))
    important_historical_figure = property(fget = lambda self: self._get_mask_bit(6),
                                        fset = lambda self, val: self._set_mask_bit(6, val))
    killed = property(fget = lambda self: self._get_mask_bit(7),
                      fset = lambda self, val: self._set_mask_bit(7, val))
    
    cleanup_1 = property(fget = lambda self: self._get_mask_bit(8),
                        fset = lambda self, val: self._set_mask_bit(8, val))
    cleanup_2 = property(fget = lambda self: self._get_mask_bit(9),
                         fset = lambda self, val: self._set_mask_bit(9, val))
    cleanup_3 = property(fget = lambda self: self._get_mask_bit(10),
                         fset = lambda self, val: self._set_mask_bit(10, val))
    for_trade = property(fget = lambda self: self._get_mask_bit(11),
                         fset = lambda self, val: self._set_mask_bit(11, val))
    
    trade_resolved = property(fget = lambda self: self._get_mask_bit(12),
                              fset = lambda self: self._set_mask_bit(12, val))
    has_breaks = property(fget = lambda self: self._get_mask_bit(13),
                          fset = lambda self, val: self._set_mask_bit(13, val))
    gutted = property(fget = lambda self: self._get_mask_bit(14),
                      fset = lambda self, val: self._set_mask_bit(14, val))
    circulatory_spray = property(fget = lambda self: self._get_mask_bit(15),
                                 fset = lambda self, val: self._set_mask_bit(15, val))
    
    locked_in_for_trading = property(fget = lambda self: self._get_mask_bit(16),
                                     fset = lambda self, val: self._set_mask_bit(16, val))
    slaughter = property(fget = lambda self: self._get_mask_bit(17),
                         fset = lambda self, val: self._set_mask_bit(17, val))
    underworld = property(fget = lambda self: self._get_mask_bit(18),
                          fset = lambda self, val: self._set_mask_bit(18, val))
    resident = property(fget = lambda self: self._get_mask_bit(19),
                        fset = lambda self, val: self._set_mask_bit(19, val))
    
    cleanup_4 = property(fget = lambda self: self._get_mask_bit(20),
                         fset = lambda self, val: self._set_mask_bit(20, val))
    calculated_insulation = property(fget = lambda self: self._get_mask_bit(21),
                                     fset = lambda self, val: self._set_mask_bit(21, val))
    visitor_uninvited = property(fget = lambda self: self._get_mask_bit(22),
                                 fset = lambda self, val: self._set_mask_bit(22, val))
    visitor = property(fget = lambda self: self._get_mask_bit(23),
                       fset = lambda self, val: self._set_mask_bit(23, val))
    
    calculated_inventory = property(fget = lambda self: self._get_mask_bit(24),
                                    fset = lambda self, val: self._set_mask_bit(24, val))
    vision_good = property(fget = lambda self: self._get_mask_bit(25),
                           fset = lambda self, val: self._set_mask_bit(25, val))
    vision_damaged = property(fget = lambda self: self._get_mask_bit(26),
                              fset = lambda self, val: self._set_mask_bit(26, val))
    vision_missing = property(fget = lambda self: self._get_mask_bit(27),
                              fset = lambda self, val: self._set_mask_bit(27, val))
    
    breathing_good = property(fget = lambda self: self._get_mask_bit(28),
                              fset = lambda self, val: self._set_mask_bit(28, val))
    breathing_problem = property(fget = lambda self: self._get_mask_bit(29),
                                 fset = lambda self, val: self._set_mask_bit(29, val))
    roaming_wilderness_population_source = property(fget = lambda self: self._get_mask_bit(30),
                                                    fset = lambda self, val: self._set_mask_bit(30, val))
    roaming_wilderness_population_source_not_a_map_feature = property(fget = lambda self: self._get_mask_bit(31),
                                                                      fset = lambda self, val: self._set_mask_bit(31, val))

class ItemFlags(_UnionBase):
    on_ground = property(fget = lambda self: self._get_mask_bit(0),
                          fset = lambda self, val: self._set_mask_bit(0, val))
    in_job = property(fget = lambda self: self._get_mask_bit(1),
                    fset = lambda self, val: self._set_mask_bit(1, val))
    in_inventory = property(fget = lambda self: self._get_mask_bit(2),
                        fset = lambda self, val: self._set_mask_bit(2, val))
    u_ngrd1 = property(fget = lambda self: self._get_mask_bit(3),
                        fset = lambda self, val: self._set_mask_bit(3, val))
    
    in_workshop = property(fget = lambda self: self._get_mask_bit(4),
                        fset = lambda self, val: self._set_mask_bit(4, val))
    u_ngrd2 = property(fget = lambda self: self._get_mask_bit(5),
                        fset = lambda self, val: self._set_mask_bit(5, val))
    u_ngrd3 = property(fget = lambda self: self._get_mask_bit(6),
                        fset = lambda self, val: self._set_mask_bit(6, val))
    rotten = property(fget = lambda self: self._get_mask_bit(7),
                      fset = lambda self, val: self._set_mask_bit(7, val))
    
    unk1 = property(fget = lambda self: self._get_mask_bit(8),
                    fset = lambda self, val: self._set_mask_bit(8, val))
    u_ngrd4 = property(fget = lambda self: self._get_mask_bit(9),
                     fset = lambda self, val: self._set_mask_bit(9, val))
    unk2 = property(fget = lambda self: self._get_mask_bit(10),
                        fset = lambda self, val: self._set_mask_bit(10, val))
    u_ngrd5 = property(fget = lambda self: self._get_mask_bit(11),
                        fset = lambda self, val: self._set_mask_bit(11, val))
    
    unk3 = property(fget = lambda self: self._get_mask_bit(12),
                      fset = lambda self: self._set_mask_bit(12, val))
    u_ngrd6 = property(fget = lambda self: self._get_mask_bit(13),
                        fset = lambda self, val: self._set_mask_bit(13, val))
    narrow = property(fget = lambda self: self._get_mask_bit(14),
                        fset = lambda self, val: self._set_mask_bit(14, val))
    u_ngrd7 = property(fget = lambda self: self._get_mask_bit(15),
                         fset = lambda self, val: self._set_mask_bit(15, val))
    
    worn = property(fget = lambda self: self._get_mask_bit(16),
                          fset = lambda self, val: self._set_mask_bit(16, val))
    unk4 = property(fget = lambda self: self._get_mask_bit(17),
                              fset = lambda self, val: self._set_mask_bit(17, val))
    u_ngrd8 = property(fget = lambda self: self._get_mask_bit(18),
                                fset = lambda self, val: self._set_mask_bit(18, val))
    forbid = property(fget = lambda self: self._get_mask_bit(19),
                              fset = lambda self, val: self._set_mask_bit(19, val))
    
    unk5 = property(fget = lambda self: self._get_mask_bit(20),
                      fset = lambda self, val: self._set_mask_bit(20, val))
    dump = property(fget = lambda self: self._get_mask_bit(21),
                               fset = lambda self, val: self._set_mask_bit(21, val))
    on_fire = property(fget = lambda self: self._get_mask_bit(22),
                       fset = lambda self, val: self._set_mask_bit(22, val))
    melt = property(fget = lambda self: self._get_mask_bit(23),
                           fset = lambda self, val: self._set_mask_bit(23, val))
    
    hidden = property(fget = lambda self: self._get_mask_bit(24),
                      fset = lambda self, val: self._set_mask_bit(24, val))
    u_ngrd9 = property(fget = lambda self: self._get_mask_bit(25),
                     fset = lambda self, val: self._set_mask_bit(25, val))
    unk6 = property(fget = lambda self: self._get_mask_bit(26),
                    fset = lambda self, val: self._set_mask_bit(26, val))
    unk7 = property(fget = lambda self: self._get_mask_bit(27),
                       fset = lambda self, val: self._set_mask_bit(27, val))
    
    unk8 = property(fget = lambda self: self._get_mask_bit(28),
                           fset = lambda self, val: self._set_mask_bit(28, val))
    unk9 = property(fget = lambda self: self._get_mask_bit(29),
                              fset = lambda self, val: self._set_mask_bit(29, val))
    unk10 = property(fget = lambda self: self._get_mask_bit(30),
                              fset = lambda self, val: self._set_mask_bit(30, val))
    unk11 = property(fget = lambda self: self._get_mask_bit(31),
                                          fset = lambda self, val: self._set_mask_bit(31, val))

dig_types = { "no" : 0,
              "default" : 1,
              "ud_stair" : 2,
              "channel" : 3,
              "ramp" : 4,
              "d_stair" : 5,
              "u_stair" : 6,
              "whatever" : 7 }

traffic_types = { "normal" : 0,
                  "low" : 1,
                  "high" : 2,
                  "restricted" : 3 }

class DesignationFlags(_UnionBase):
    def dig(self, dig_type):
        if dig_type == "no":
            self.dig_1 = False; self.dig_2 = False; self.dig_3 = False
        elif dig_type == "default":
            self.dig_1 = True; self.dig_2 = False; self.dig_3 = False
        elif dig_type == "ud_stair":
            self.dig_1 = False; self.dig_2 = True; self.dig_3 = False
        elif dig_type == "channel":
            self.dig_1 = True; self.dig_2 = True; self.dig_3 = False
        elif dig_type == "ramp":
            self.dig_1 = False; self.dig_2 = False; self.dig_3 = True
        elif dig_type == "d_stair":
            self.dig_1 = True; self.dig_2 = False; self.dig_3 = True
        elif dig_type == "u_stair":
            self.dig_1 = False; self.dig_2 = True; self.dig_3 = True
        elif dig_type == "whatever":
            self.dig_1 = True; self.dig_2 = True; self.dig_3 = True

    def set_traffic(self, traffic_type):
        if traffic_type == "normal":
            self.traffic_1 = False; self.traffic_2 = False
        elif traffic_type == "low":
            self.traffic_1 = True; self.traffic_2 = False
        elif traffic_type == "high":
            self.traffic_1 = False; self.traffic_2 = True
        elif traffic_type == "restricted":
            self.traffic_1 = True; self.traffic_2 = True
    
    flow_size_1 = property(fget = lambda self: self._get_mask_bit(0),
                          fset = lambda self, val: self._set_mask_bit(0, val))
    flow_size_2 = property(fget = lambda self: self._get_mask_bit(1),
                    fset = lambda self, val: self._set_mask_bit(1, val))
    flow_size_3 = property(fget = lambda self: self._get_mask_bit(2),
                        fset = lambda self, val: self._set_mask_bit(2, val))
    pile = property(fget = lambda self: self._get_mask_bit(3),
                        fset = lambda self, val: self._set_mask_bit(3, val))
    
    dig_1 = property(fget = lambda self: self._get_mask_bit(4),
                        fset = lambda self, val: self._set_mask_bit(4, val))
    dig_2 = property(fget = lambda self: self._get_mask_bit(5),
                        fset = lambda self, val: self._set_mask_bit(5, val))
    dig_3 = property(fget = lambda self: self._get_mask_bit(6),
                        fset = lambda self, val: self._set_mask_bit(6, val))
    smooth_1 = property(fget = lambda self: self._get_mask_bit(7),
                      fset = lambda self, val: self._set_mask_bit(7, val))
    
    smooth_2 = property(fget = lambda self: self._get_mask_bit(8),
                    fset = lambda self, val: self._set_mask_bit(8, val))
    hidden = property(fget = lambda self: self._get_mask_bit(9),
                      fset = lambda self, val: self._set_mask_bit(9, val))
    geolayer_index_1 = property(fget = lambda self: self._get_mask_bit(10),
                     fset = lambda self, val: self._set_mask_bit(10, val))
    geolayer_index_2 = property(fget = lambda self: self._get_mask_bit(11),
                        fset = lambda self, val: self._set_mask_bit(11, val))
    
    geolayer_index_3 = property(fget = lambda self: self._get_mask_bit(12),
                        fset = lambda self, val: self._set_mask_bit(12, val))    
    geolayer_index_4 = property(fget = lambda self: self._get_mask_bit(13),
                      fset = lambda self: self._set_mask_bit(13, val))
    light = property(fget = lambda self: self._get_mask_bit(14),
                        fset = lambda self, val: self._set_mask_bit(14, val))
    subterranean = property(fget = lambda self: self._get_mask_bit(15),
                        fset = lambda self, val: self._set_mask_bit(15, val))
    
    skyview = property(fget = lambda self: self._get_mask_bit(16),
                         fset = lambda self, val: self._set_mask_bit(16, val))    
    biome_1 = property(fget = lambda self: self._get_mask_bit(17),
                          fset = lambda self, val: self._set_mask_bit(17, val))
    biome_2 = property(fget = lambda self: self._get_mask_bit(18),
                              fset = lambda self, val: self._set_mask_bit(18, val))
    biome_3 = property(fget = lambda self: self._get_mask_bit(19),
                                fset = lambda self, val: self._set_mask_bit(19, val))
    
    biome_4 = property(fget = lambda self: self._get_mask_bit(20),
                              fset = lambda self, val: self._set_mask_bit(20, val))    
    liquid_type = property(fget = lambda self: self._get_mask_bit(21),
                      fset = lambda self, val: self._set_mask_bit(21, val))
    water_table = property(fget = lambda self: self._get_mask_bit(22),
                               fset = lambda self, val: self._set_mask_bit(22, val))
    rained = property(fget = lambda self: self._get_mask_bit(23),
                       fset = lambda self, val: self._set_mask_bit(23, val))
    
    traffic_1 = property(fget = lambda self: self._get_mask_bit(24),
                           fset = lambda self, val: self._set_mask_bit(24, val))    
    traffic_2 = property(fget = lambda self: self._get_mask_bit(25),
                      fset = lambda self, val: self._set_mask_bit(25, val))
    flow_forbid = property(fget = lambda self: self._get_mask_bit(26),
                     fset = lambda self, val: self._set_mask_bit(26, val))
    liquid_static = property(fget = lambda self: self._get_mask_bit(27),
                    fset = lambda self, val: self._set_mask_bit(27, val))
    
    moss = property(fget = lambda self: self._get_mask_bit(28),
                       fset = lambda self, val: self._set_mask_bit(28, val))    
    feature_present = property(fget = lambda self: self._get_mask_bit(29),
                           fset = lambda self, val: self._set_mask_bit(29, val))
    liquid_character_1 = property(fget = lambda self: self._get_mask_bit(30),
                              fset = lambda self, val: self._set_mask_bit(30, val))
    liquid_character_2 = property(fget = lambda self: self._get_mask_bit(31),
                              fset = lambda self, val: self._set_mask_bit(31, val))

class OccupancyFlags(_UnionBase):
    def set_splatter(self, state):
        if state:
            self.whole &= 0xFFFFFFC0
        else:
            self.whole &= ~0xFFFFFFC0
    building_1 = property(fget = lambda self: self._get_mask_bit(0),
                          fset = lambda self, val: self._set_mask_bit(0, val))
    building_2 = property(fget = lambda self: self._get_mask_bit(1),
                    fset = lambda self, val: self._set_mask_bit(1, val))
    building_3 = property(fget = lambda self: self._get_mask_bit(2),
                        fset = lambda self, val: self._set_mask_bit(2, val))
    unit = property(fget = lambda self: self._get_mask_bit(3),
                        fset = lambda self, val: self._set_mask_bit(3, val))
    
    unit_grounded = property(fget = lambda self: self._get_mask_bit(4),
                        fset = lambda self, val: self._set_mask_bit(4, val))
    item = property(fget = lambda self: self._get_mask_bit(5),
                        fset = lambda self, val: self._set_mask_bit(5, val))
    mud = property(fget = lambda self: self._get_mask_bit(6),
                        fset = lambda self, val: self._set_mask_bit(6, val))
    vomit = property(fget = lambda self: self._get_mask_bit(7),
                      fset = lambda self, val: self._set_mask_bit(7, val))
    
    broken_arrows_color_1 = property(fget = lambda self: self._get_mask_bit(8),
                    fset = lambda self, val: self._set_mask_bit(8, val))
    broken_arrows_color_2 = property(fget = lambda self: self._get_mask_bit(9),
                     fset = lambda self, val: self._set_mask_bit(9, val))
    broken_arrows_color_3 = property(fget = lambda self: self._get_mask_bit(10),
                        fset = lambda self, val: self._set_mask_bit(10, val))
    broken_arrows_color_4 = property(fget = lambda self: self._get_mask_bit(11),
                        fset = lambda self, val: self._set_mask_bit(11, val))
    
    blood_g = property(fget = lambda self: self._get_mask_bit(12),
                      fset = lambda self: self._set_mask_bit(12, val))
    blood_g2 = property(fget = lambda self: self._get_mask_bit(13),
                        fset = lambda self, val: self._set_mask_bit(13, val))
    blood_b = property(fget = lambda self: self._get_mask_bit(14),
                        fset = lambda self, val: self._set_mask_bit(14, val))
    blood_b2 = property(fget = lambda self: self._get_mask_bit(15),
                         fset = lambda self, val: self._set_mask_bit(15, val))
    
    blood_y = property(fget = lambda self: self._get_mask_bit(16),
                          fset = lambda self, val: self._set_mask_bit(16, val))
    blood_y2 = property(fget = lambda self: self._get_mask_bit(17),
                              fset = lambda self, val: self._set_mask_bit(17, val))
    blood_m = property(fget = lambda self: self._get_mask_bit(18),
                                fset = lambda self, val: self._set_mask_bit(18, val))
    blood_m2 = property(fget = lambda self: self._get_mask_bit(19),
                              fset = lambda self, val: self._set_mask_bit(19, val))
    
    blood_c = property(fget = lambda self: self._get_mask_bit(20),
                      fset = lambda self, val: self._set_mask_bit(20, val))
    blood_c2 = property(fget = lambda self: self._get_mask_bit(21),
                               fset = lambda self, val: self._set_mask_bit(21, val))
    blood_w = property(fget = lambda self: self._get_mask_bit(22),
                       fset = lambda self, val: self._set_mask_bit(22, val))
    blood_w2 = property(fget = lambda self: self._get_mask_bit(23),
                           fset = lambda self, val: self._set_mask_bit(23, val))
    
    blood_o = property(fget = lambda self: self._get_mask_bit(24),
                      fset = lambda self, val: self._set_mask_bit(24, val))
    blood_o2 = property(fget = lambda self: self._get_mask_bit(25),
                     fset = lambda self, val: self._set_mask_bit(25, val))
    slime = property(fget = lambda self: self._get_mask_bit(26),
                    fset = lambda self, val: self._set_mask_bit(26, val))
    slime2 = property(fget = lambda self: self._get_mask_bit(27),
                       fset = lambda self, val: self._set_mask_bit(27, val))
    
    blood = property(fget = lambda self: self._get_mask_bit(28),
                           fset = lambda self, val: self._set_mask_bit(28, val))
    blood2 = property(fget = lambda self: self._get_mask_bit(29),
                              fset = lambda self, val: self._set_mask_bit(29, val))
    broken_arrows_variant = property(fget = lambda self: self._get_mask_bit(30),
                              fset = lambda self, val: self._set_mask_bit(30, val))
    snow = property(fget = lambda self: self._get_mask_bit(31),
                                          fset = lambda self, val: self._set_mask_bit(31, val))
