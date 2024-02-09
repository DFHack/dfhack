#include "df/item_actual.h"

static int map_temp_mult = -1;
static int max_heat_ticks = 0;

struct fast_heat_hook : df::item_actual {
    typedef df::item_actual interpose_base;

    DEFINE_VMETHOD_INTERPOSE(
        bool, updateTempFromMap,
        (bool local, bool contained, bool adjust, int32_t rate_mult)
    ) {
        int cmult = map_temp_mult;
        map_temp_mult = rate_mult;

        bool rv = INTERPOSE_NEXT(updateTempFromMap)(local, contained, adjust, rate_mult);
        map_temp_mult = cmult;
        return rv;
    }

    DEFINE_VMETHOD_INTERPOSE(
        bool, updateTemperature,
        (uint16_t temp, bool local, bool contained, bool adjust, int32_t rate_mult)
    ) {
        // Some items take ages to cross the last degree, so speed them up
        if (map_temp_mult > 0 && temp != temperature.whole && max_heat_ticks > 0) {
            int spec = getSpecHeat();
            if (spec != 60001)
                rate_mult = std::max(map_temp_mult, spec/max_heat_ticks/abs(temp - temperature.whole));
        }

        return INTERPOSE_NEXT(updateTemperature)(temp, local, contained, adjust, rate_mult);
    }

    DEFINE_VMETHOD_INTERPOSE(bool, adjustTemperature, (uint16_t temp, int32_t rate_mult)) {
        if (map_temp_mult > 0)
            rate_mult = map_temp_mult;

        return INTERPOSE_NEXT(adjustTemperature)(temp, rate_mult);
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(fast_heat_hook, updateTempFromMap);
IMPLEMENT_VMETHOD_INTERPOSE(fast_heat_hook, updateTemperature);
IMPLEMENT_VMETHOD_INTERPOSE(fast_heat_hook, adjustTemperature);
