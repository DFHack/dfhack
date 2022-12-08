#pragma once
#include <Debug.h>

namespace DFHack {
    DBG_EXTERN(channelsafely, monitor);
    DBG_EXTERN(channelsafely, manager);
    DBG_EXTERN(channelsafely, groups);
    DBG_EXTERN(channelsafely, jobs);
}

struct Configuration {
    bool riskaverse = true;
    bool monitoring = false;
    bool require_vision = true;
    bool insta_dig = false;
    bool resurrect = false;
    int32_t refresh_freq = 600;
    int32_t monitor_freq = 1;
    uint8_t ignore_threshold = 5;
    uint8_t fall_threshold = 1;
};

extern Configuration config;
extern int32_t mapx, mapy, mapz;
