#pragma once

#include <VTableInterpose.h>
#include "PluginManager.h"

#include "DataDefs.h"
#include "df/viewscreen_choose_start_sitest.h"

#include "defs.h"

using df::global::enabler;
using df::global::gps;

namespace embark_assist {
    namespace overlay {
        typedef void(*embark_update_callbacks)();
        typedef void(*match_callbacks)();
        typedef void(*clear_match_callbacks)();
        typedef void(*shutdown_callbacks)();

        bool setup(DFHack::Plugin *plugin_self,
            embark_update_callbacks embark_update_callback,
            match_callbacks match_callback,
            clear_match_callbacks clear_match_callback,
            embark_assist::defs::find_callbacks find_callback,
            shutdown_callbacks shutdown_callback,
            uint16_t max_inorganic);

        void set_sites(embark_assist::defs::site_lists *site_list);
        void initiate_match();
        void match_progress(uint16_t count, embark_assist::defs::match_results *match_results, bool done);
        void set_embark(embark_assist::defs::site_infos *site_info);
        void set_mid_level_tile_match(embark_assist::defs::mlt_matches mlt_matches);
        void clear_match_results();
        void shutdown();
    }
}