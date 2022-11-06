#pragma once
#include <Debug.h>

namespace DFHack {
    DBG_EXTERN(channelsafely, monitor);
    DBG_EXTERN(channelsafely, manager);
    DBG_EXTERN(channelsafely, groups);
    DBG_EXTERN(channelsafely, jobs);
}

struct Configuration {
    bool debug = false;
    bool monitor_active = false;
    bool require_vision = true;
    bool insta_dig = false;
    int32_t refresh_freq = 600;
    int32_t monitor_freq = 10;
    uint8_t ignore_threshold = 7;
    uint8_t fall_threshold = 1;
};

extern Configuration config;
extern int32_t mapx, mapy, mapz;
