#include "Core.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>

#include <time.h>
#include <modules/Gui.h>
#include <modules/Screen.h>

#include "DataDefs.h"
#include "df/coord2d.h"
#include "df/inorganic_flags.h"
#include "df/inorganic_raw.h"
#include "df/interfacest.h"
#include "df/viewscreen.h"
#include "df/viewscreen_choose_start_sitest.h"
#include "df/world.h"
#include "df/world_data.h"
#include "df/world_geo_biome.h"
#include "df/world_raws.h"

#include "defs.h"
#include "embark-assistant.h"
#include "finder_ui.h"
#include "matcher.h"
#include "overlay.h"
#include "survey.h"

DFHACK_PLUGIN("embark-assistant");

using namespace DFHack;
using namespace df::enums;
using namespace Gui;

REQUIRE_GLOBAL(world);

namespace embark_assist {
    namespace main {
        struct states {
            embark_assist::defs::geo_data geo_summary;
            embark_assist::defs::world_tile_data survey_results;
            embark_assist::defs::site_lists region_sites;
            embark_assist::defs::site_infos site_info;
            embark_assist::defs::match_results match_results;
            embark_assist::defs::match_iterators match_iterator;
            uint16_t max_inorganic;
        };

        static states *state = nullptr;

        void embark_update ();
        void shutdown();

        //===============================================================================

        void embark_update() {
            auto screen = Gui::getViewscreenByType<df::viewscreen_choose_start_sitest>(0);
            embark_assist::defs::mid_level_tiles mlt;
            embark_assist::survey::initiate(&mlt);

            embark_assist::survey::survey_mid_level_tile(&state->geo_summary,
                &state->survey_results,
                &mlt);

            embark_assist::survey::survey_embark(&mlt, &state->site_info, false);
            embark_assist::overlay::set_embark(&state->site_info);

            embark_assist::survey::survey_region_sites(&state->region_sites);
            embark_assist::overlay::set_sites(&state->region_sites);

            embark_assist::overlay::set_mid_level_tile_match(state->match_results.at(screen->location.region_pos.x).at(screen->location.region_pos.y).mlt_match);
        }

        //===============================================================================

        void match() {
//            color_ostream_proxy out(Core::getInstance().getConsole());

            uint16_t count = embark_assist::matcher::find(&state->match_iterator,
                &state->geo_summary,
                &state->survey_results,
                &state->match_results);

            embark_assist::overlay::match_progress(count, &state->match_results, !state->match_iterator.active);

            if (!state->match_iterator.active) {
                auto screen = Gui::getViewscreenByType<df::viewscreen_choose_start_sitest>(0);
                embark_assist::overlay::set_mid_level_tile_match(state->match_results.at(screen->location.region_pos.x).at(screen->location.region_pos.y).mlt_match);
            }
        }

        //===============================================================================

        void clear_match() {
//            color_ostream_proxy out(Core::getInstance().getConsole());
            if (state->match_iterator.active) {
                embark_assist::matcher::move_cursor(state->match_iterator.x, state->match_iterator.y);
            }
            embark_assist::survey::clear_results(&state->match_results);
            embark_assist::overlay::clear_match_results();
            embark_assist::main::state->match_iterator.active = false;
        }

        //===============================================================================

        void find(embark_assist::defs::finders finder) {
//            color_ostream_proxy out(Core::getInstance().getConsole());

            state->match_iterator.x = embark_assist::survey::get_last_pos().x;
            state->match_iterator.y = embark_assist::survey::get_last_pos().y;
            state->match_iterator.finder = finder;
            embark_assist::overlay::initiate_match();
        }

        //===============================================================================

        void shutdown() {
//            color_ostream_proxy out(Core::getInstance().getConsole());
            embark_assist::survey::shutdown();
            embark_assist::finder_ui::shutdown();
            embark_assist::overlay::shutdown();
            delete state;
            state = nullptr;
        }
    }
}

//=======================================================================================

command_result embark_assistant (color_ostream &out, std::vector <std::string> & parameters);

//=======================================================================================

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "embark-assistant", "Embark site selection support.",
        embark_assistant, true, /* true means that the command can't be used from non-interactive user interface */
        // Extended help string. Used by CR_WRONG_USAGE and the help command:
        "  This command starts the embark-assist plugin that provides embark site\n"
        "  selection help. It has to be called while the pre-embark screen is\n"
        "  displayed and shows extended (and correct(?)) resource information for\n"
        "  the embark rectangle as well as normally undisplayed sites in the\n"
        "  current embark region. It also has a site selection tool with more\n"
        "  options than DF's vanilla search tool. For detailed help invoke the\n"
        "  in game info screen. Requires 42 lines to display properly.\n"
    ));
    return CR_OK;
}

//=======================================================================================

DFhackCExport command_result plugin_shutdown (color_ostream &out)
{
    return CR_OK;
}

//=======================================================================================

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event) {
      case DFHack::SC_UNKNOWN:
          break;

      case DFHack::SC_WORLD_LOADED:
          break;

      case DFHack::SC_WORLD_UNLOADED:
      case DFHack::SC_MAP_LOADED:
          if (embark_assist::main::state) {
              embark_assist::main::shutdown();
          }
          break;

      case DFHack::SC_MAP_UNLOADED:
          break;

      case DFHack::SC_VIEWSCREEN_CHANGED:
          break;

      case DFHack::SC_CORE_INITIALIZED:
          break;

      case DFHack::SC_BEGIN_UNLOAD:
          break;

      case DFHack::SC_PAUSED:
          break;

      case DFHack::SC_UNPAUSED:
          break;
    }
    return CR_OK;
}


//=======================================================================================

command_result embark_assistant(color_ostream &out, std::vector <std::string> & parameters)
{
    if (!parameters.empty())
        return CR_WRONG_USAGE;

    CoreSuspender suspend;

    auto screen = Gui::getViewscreenByType<df::viewscreen_choose_start_sitest>(0);
    if (!screen) {
        out.printerr("This plugin works only in the embark site selection phase.\n");
        return CR_WRONG_USAGE;
    }

    df::world_data *world_data = world->world_data;

    if (embark_assist::main::state) {
        out.printerr("You can't invoke the embark assistant while it's already active.\n");
        return CR_WRONG_USAGE;
    }

    embark_assist::main::state = new embark_assist::main::states;

    embark_assist::main::state->match_iterator.active = false;

    //  Find the end of the normal inorganic definitions.
    embark_assist::main::state->max_inorganic = 0;
    for (uint16_t i = 0; i < world->raws.inorganics.size(); i++) {
        if (world->raws.inorganics[i]->flags.is_set(df::inorganic_flags::GENERATED)) embark_assist::main::state->max_inorganic = i;
    }
    embark_assist::main::state->max_inorganic++;  //  To allow it to be used as size() replacement

    if (!embark_assist::overlay::setup(plugin_self,
        embark_assist::main::embark_update,
        embark_assist::main::match,
        embark_assist::main::clear_match,
        embark_assist::main::find,
        embark_assist::main::shutdown,
        embark_assist::main::state->max_inorganic)) {
        return CR_FAILURE;
    }

    embark_assist::survey::setup(embark_assist::main::state->max_inorganic);
    embark_assist::main::state->geo_summary.resize(world_data->geo_biomes.size());
    embark_assist::main::state->survey_results.resize(world->worldgen.worldgen_parms.dim_x);

    for (uint16_t i = 0; i < world->worldgen.worldgen_parms.dim_x; i++) {
        embark_assist::main::state->survey_results[i].resize(world->worldgen.worldgen_parms.dim_y);

        for (uint16_t k = 0; k < world->worldgen.worldgen_parms.dim_y; k++) {
            embark_assist::main::state->survey_results[i][k].surveyed = false;
            embark_assist::main::state->survey_results[i][k].aquifer_count = 0;
            embark_assist::main::state->survey_results[i][k].clay_count = 0;
            embark_assist::main::state->survey_results[i][k].sand_count = 0;
            embark_assist::main::state->survey_results[i][k].flux_count = 0;
            embark_assist::main::state->survey_results[i][k].min_region_soil = 10;
            embark_assist::main::state->survey_results[i][k].max_region_soil = 0;
            embark_assist::main::state->survey_results[i][k].waterfall = false;
            embark_assist::main::state->survey_results[i][k].river_size = embark_assist::defs::river_sizes::None;

            for (uint8_t l = 1; l < 10; l++) {
                embark_assist::main::state->survey_results[i][k].biome_index[l] = -1;
                embark_assist::main::state->survey_results[i][k].biome[l] = -1;
                embark_assist::main::state->survey_results[i][k].evil_weather[l] = false;
                embark_assist::main::state->survey_results[i][k].reanimating[l] = false;
                embark_assist::main::state->survey_results[i][k].thralling[l] = false;
            }

            for (uint8_t l = 0; l < 2; l++) {
                embark_assist::main::state->survey_results[i][k].savagery_count[l] = 0;
                embark_assist::main::state->survey_results[i][k].evilness_count[l] = 0;
            }
            embark_assist::main::state->survey_results[i][k].metals.resize(embark_assist::main::state->max_inorganic);
            embark_assist::main::state->survey_results[i][k].economics.resize(embark_assist::main::state->max_inorganic);
            embark_assist::main::state->survey_results[i][k].minerals.resize(embark_assist::main::state->max_inorganic);
        }
    }

    embark_assist::survey::high_level_world_survey(&embark_assist::main::state->geo_summary,
        &embark_assist::main::state->survey_results);

    embark_assist::main::state->match_results.resize(world->worldgen.worldgen_parms.dim_x);

    for (uint16_t i = 0; i < world->worldgen.worldgen_parms.dim_x; i++) {
        embark_assist::main::state->match_results[i].resize(world->worldgen.worldgen_parms.dim_y);
    }

    embark_assist::survey::clear_results(&embark_assist::main::state->match_results);
    embark_assist::survey::survey_region_sites(&embark_assist::main::state->region_sites);
    embark_assist::overlay::set_sites(&embark_assist::main::state->region_sites);

    embark_assist::defs::mid_level_tiles mlt;
    embark_assist::survey::survey_mid_level_tile(&embark_assist::main::state->geo_summary, &embark_assist::main::state->survey_results, &mlt);
    embark_assist::survey::survey_embark(&mlt, &embark_assist::main::state->site_info, false);
    embark_assist::overlay::set_embark(&embark_assist::main::state->site_info);

    return CR_OK;
}
