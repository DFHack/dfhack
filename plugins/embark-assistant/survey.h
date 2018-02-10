#pragma once
#include <map>

#include "DataDefs.h"
#include "df/coord2d.h"

#include "defs.h"

using namespace DFHack;

namespace embark_assist {
    namespace survey {
        void setup(uint16_t max_inorganic);

        df::coord2d get_last_pos();

        void initiate(embark_assist::defs::mid_level_tiles *mlt);

        void clear_results(embark_assist::defs::match_results *match_results);

        void high_level_world_survey(embark_assist::defs::geo_data *geo_summary,
            embark_assist::defs::world_tile_data *survey_results);

        void survey_mid_level_tile(embark_assist::defs::geo_data *geo_summary,
            embark_assist::defs::world_tile_data *survey_results,
            embark_assist::defs::mid_level_tiles *mlt);

        df::coord2d apply_offset(uint16_t x, uint16_t y, int8_t offset);

        void survey_region_sites(embark_assist::defs::site_lists *site_list);

        void survey_embark(embark_assist::defs::mid_level_tiles *mlt,
            embark_assist::defs::site_infos *site_info,
            bool use_cache);

        void shutdown();
    }
}