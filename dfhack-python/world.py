from ctypes import *
from dftypes import libdfhack, GameModes
from util import _uintify, uint_ptr

libdfhack.World_ReadGameMode.argtypes = [ c_void_p, POINTER(GameModes) ]

class World(object):
    def __init__(self, ptr):
        self._world_ptr = ptr
    
    def start(self):
        return libdfhack.World_Start(self._world_ptr) > 0
    
    def finish(self):
        return libdfhack.World_Finish(self._world_ptr) > 0
    
    def read_pause_state(self):
        return libdfhack.World_ReadPauseState(self._world_ptr) > 0
    
    def set_pause_state(self, pause_state):
        p = c_byte(0)
        
        if pause_state is not None and pause_state is not False:
            p.value = 1
        
        return libdfhack.World_SetPauseState(self._world_ptr, p) > 0
    
    def read_current_tick(self):
        tick = c_uint(0)
        
        if libdfhack.World_ReadCurrentTick(self._world_ptr, byref(tick)) > 0:
            return int(tick)
        else:
            return -1
    
    def read_current_year(self):
        year = c_uint(0)
        
        if libdfhack.World_ReadCurrentYear(self._world_ptr, byref(year)) > 0:
            return int(year)
        else:
            return -1
    
    def read_current_month(self):
        month = c_uint(0)
        
        if libdfhack.World_ReadCurrentMonth(self._world_ptr, byref(month)) > 0:
            return int(month)
        else:
            return -1
    
    def read_current_day(self):
        day = c_uint(0)
        
        if libdfhack.World_ReadCurrentDay(self._world_ptr, byref(day)) > 0:
            return int(day)
        else:
            return -1
    
    def read_current_weather(self):
        weather = c_ubyte(0)
        
        if libdfhack.World_ReadCurrentWeather(self._world_ptr, byref(weather)) > 0:
            return int(weather)
        else:
            return -1
    
    def write_current_weather(self, weather):
        return libdfhack.World_WriteCurrentWeather(self._world_ptr, c_ubyte(weather))
    
    def read_game_mode(self):
        game_modes = GameModes()
        
        if libdfhack.World_ReadGameMode(self._world_ptr, byref(game_modes)) > 0:
             return game_modes
        else:
            return None
    
    def write_game_mode(self, game_mode):
        return libdfhack.World_WriteGameMode(self._world_ptr, game_modes) > 0