#pragma once

#include "DataDefs.h"

#include "defs.h"

using namespace DFHack;

namespace embark_assist {
    namespace matcher {
        void move_cursor(uint16_t x, uint16_t y);

        //  Used to iterate over the whole world to generate a map of world tiles
        //  that contain matching embarks.
        //
        uint16_t find(embark_assist::defs::match_iterators *iterator,
            embark_assist::defs::geo_data *geo_summary,
            embark_assist::defs::world_tile_data *survey_results,
            embark_assist::defs::match_results *match_results);
    }
}