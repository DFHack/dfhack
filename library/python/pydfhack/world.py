from ctypes import *
from util import _uintify, uint_ptr

class World(object):
    def __init__(self, ptr):
        self._world_ptr = ptr
    
    def start(self):
        return libdfhack.World_Start(self._world_ptr) > 0
    
    def finish(self):
        return libdfhack.World_Finish(self._world_ptr) > 0
    
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
        return int(libdfhack.World_ReadGameMode(self._world_ptr))