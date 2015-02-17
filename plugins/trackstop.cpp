/*
 * Trackstop plugin.
 * Shows track stop friction and direction in its 'q' menu.
 */

#include "uicommon.h"
#include "LuaTools.h"

#include "df/building_rollersst.h"
#include "df/building_trapst.h"
#include "df/job.h"
#include "df/viewscreen_dwarfmodest.h"

#include "modules/Gui.h"

using namespace DFHack;
using namespace std;

using df::building_rollersst;
using df::building_trapst;
using df::enums::trap_type::trap_type;
using df::enums::screw_pump_direction::screw_pump_direction;

DFHACK_PLUGIN("trackstop");
#define AUTOENABLE false
DFHACK_PLUGIN_IS_ENABLED(enabled);

REQUIRE_GLOBAL(gps);
REQUIRE_GLOBAL(ui);
REQUIRE_GLOBAL(world);

/*
 * Interface hooks
 */
struct trackstop_hook : public df::viewscreen_dwarfmodest {
    typedef df::viewscreen_dwarfmodest interpose_base;

    enum Friction {
        Lowest = 10,
        Low = 50,
        Medium = 500,
        High = 10000,
        Highest = 50000
    };

    building_trapst *get_selected_trackstop() {
        if (ui->main.mode != ui_sidebar_mode::QueryBuilding) {
            // Not in a building's 'q' menu.
            return nullptr;
        }

        building_trapst *ts = virtual_cast<building_trapst>(world->selected_building);
        if (!ts) {
            // Not a trap type of building.
            return nullptr;
        }

        if (ts->trap_type != df::trap_type::TrackStop) {
            // Not a trackstop.
            return nullptr;
        }

        if (ts->construction_stage < ts->getMaxBuildStage()) {
            // Not yet fully constructed.
            return nullptr;
        }

        for (auto it = ts->jobs.begin(); it != ts->jobs.end(); it++) {
            auto job = *it;
            if (job->job_type == df::job_type::DestroyBuilding) {
                // Slated for removal.
                return nullptr;
            }
        }

        return ts;
    }

    bool handleInput(set<df::interface_key> *input) {
        building_trapst *ts = get_selected_trackstop();
        if (!ts) {
            return false;
        }

        if (input->count(interface_key::BUILDING_TRACK_STOP_DUMP)) {
            // Change track stop dump direction.
            // There might be a more elegant way to do this.

            if (!ts->use_dump) {
                // No -> North
                ts->use_dump = 1;
                ts->dump_x_shift = 0;
                ts->dump_y_shift = -1;
            } else if (ts->dump_x_shift == 0 && ts->dump_y_shift == -1) {
                // North -> South
                ts->dump_x_shift = 0;
                ts->dump_y_shift = 1;
            } else if (ts->dump_x_shift == 0 && ts->dump_y_shift == 1) {
                // South -> East
                ts->dump_x_shift = 1;
                ts->dump_y_shift = 0;
            } else if (ts->dump_x_shift == 1 && ts->dump_y_shift == 0) {
                // East -> West
                ts->dump_x_shift = -1;
                ts->dump_y_shift = 0;
            } else {
                // West (or Elsewhere) -> No
                ts->use_dump = 0;
                ts->dump_x_shift = 0;
                ts->dump_y_shift = 0;
            }

            return true;
        } else if (input->count(interface_key::BUILDING_TRACK_STOP_FRICTION_UP)) {
            ts->friction = (
                (ts->friction < Friction::Lowest)?  Friction::Lowest:
                (ts->friction < Friction::Low)?     Friction::Low:
                (ts->friction < Friction::Medium)?  Friction::Medium:
                (ts->friction < Friction::High)?    Friction::High:
                (ts->friction < Friction::Highest)? Friction::Highest:
                ts->friction
            );

            return true;
        } else if (input->count(interface_key::BUILDING_TRACK_STOP_FRICTION_DOWN)) {
            ts->friction = (
                (ts->friction > Friction::Highest)? Friction::Highest:
                (ts->friction > Friction::High)?    Friction::High:
                (ts->friction > Friction::Medium)?  Friction::Medium:
                (ts->friction > Friction::Low)?     Friction::Low:
                (ts->friction > Friction::Lowest)?  Friction::Lowest:
                ts->friction
            );

            return true;
        }

        return false;
    }

    DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input)) {
        if (!handleInput(input)) {
            INTERPOSE_NEXT(feed)(input);
        }
    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ()) {
        INTERPOSE_NEXT(render)();

        building_trapst *ts = get_selected_trackstop();
        if (ts) {
            auto dims = Gui::getDwarfmodeViewDims();
            int left_margin = dims.menu_x1 + 1;
            int x = left_margin;
            int y = dims.y1 + 1;

            OutputString(COLOR_WHITE, x, y, "Track Stop", true, left_margin);

            y += 3;
            OutputString(COLOR_WHITE, x, y, "Friction: ", false);
            OutputString(COLOR_WHITE, x, y, (
                (ts->friction <= Friction::Lowest)? "Lowest":
                (ts->friction <= Friction::Low)?    "Low":
                (ts->friction <= Friction::Medium)? "Medium":
                (ts->friction <= Friction::High)?   "High":
                "Highest"
            ), true, left_margin);
            OutputString(COLOR_LIGHTRED, x, y, Screen::getKeyDisplay(interface_key::BUILDING_TRACK_STOP_FRICTION_DOWN));
            OutputString(COLOR_LIGHTRED, x, y, Screen::getKeyDisplay(interface_key::BUILDING_TRACK_STOP_FRICTION_UP));
            OutputString(COLOR_WHITE, x, y, ": Change Friction", true, left_margin);

            y += 1;

            OutputString(COLOR_WHITE, x, y, "Dump on arrival: ", false);
            OutputString(COLOR_WHITE, x, y, (
                (!ts->use_dump)? "No":
                (ts->dump_x_shift ==  0 && ts->dump_y_shift == -1)? "North":
                (ts->dump_x_shift ==  0 && ts->dump_y_shift ==  1)? "South":
                (ts->dump_x_shift ==  1 && ts->dump_y_shift ==  0)? "East":
                (ts->dump_x_shift == -1 && ts->dump_y_shift ==  0)? "West":
                "Elsewhere"
            ), true, left_margin);
            OutputString(COLOR_LIGHTRED, x, y, Screen::getKeyDisplay(interface_key::BUILDING_TRACK_STOP_DUMP));
            OutputString(COLOR_WHITE, x, y, ": Activate/change direction", true, left_margin);
            y += 1;
            OutputString(COLOR_GREY, x, y, "DFHack");
        }
    }
};

struct roller_hook : public df::viewscreen_dwarfmodest {
    typedef df::viewscreen_dwarfmodest interpose_base;

    enum Speed {
        Lowest  = 10000,
        Low     = 20000,
        Medium  = 30000,
        High    = 40000,
        Highest = 50000
    };

    building_rollersst *get_selected_roller() {
        if (ui->main.mode != ui_sidebar_mode::QueryBuilding) {
            // Not in a building's 'q' menu.
            return nullptr;
        }

        building_rollersst *roller = virtual_cast<building_rollersst>(world->selected_building);
        if (!roller) {
            // Not a roller.
            return nullptr;
        }

        if (roller->construction_stage < roller->getMaxBuildStage()) {
            // Not yet fully constructed.
            return nullptr;
        }

        for (auto it = roller->jobs.begin(); it != roller->jobs.end(); it++) {
            auto job = *it;
            if (job->job_type == df::job_type::DestroyBuilding) {
                // Slated for removal.
                return nullptr;
            }
        }

        return roller;
    }

    bool handleInput(set<df::interface_key> *input) {
        building_rollersst *roller = get_selected_roller();
        if (!roller) {
            return false;
        }

        if (input->count(interface_key::BUILDING_ORIENT_NONE)) {
            // Flip roller orientation.
            // Long rollers can only be oriented along their length.
            // Todo: Only add 1 to 1x1 rollers: x ^= ((x&1)<<1)|1
            // Todo: This could have been elegant without all the casting,
            // but as an enum it might be better off listing each case.
            roller->direction = (df::enums::screw_pump_direction::screw_pump_direction)(((int8_t)roller->direction) ^ 2);
            return true;
        } else if (input->count(interface_key::BUILDING_ROLLERS_SPEED_UP)) {
            if (roller->speed < Speed::Highest) {
                roller->speed += Speed::Lowest;
            }

            return true;
        } else if (input->count(interface_key::BUILDING_ROLLERS_SPEED_DOWN)) {
            if (roller->speed > Speed::Lowest) {
                roller->speed -= Speed::Lowest;
            }

            return true;
        }

        return false;
    }

    DEFINE_VMETHOD_INTERPOSE(void, feed, (set<df::interface_key> *input)) {
        if (!handleInput(input)) {
            INTERPOSE_NEXT(feed)(input);
        }
    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ()) {
        INTERPOSE_NEXT(render)();

        building_rollersst *roller = get_selected_roller();
        if (roller) {
            auto dims = Gui::getDwarfmodeViewDims();
            int left_margin = dims.menu_x1 + 1;
            int x = left_margin;
            int y = dims.y1 + 6;

            OutputString(COLOR_LIGHTRED, x, y, Screen::getKeyDisplay(interface_key::BUILDING_ORIENT_NONE));
            OutputString(COLOR_WHITE, x, y, ": Rolls ", false);
            OutputString(COLOR_WHITE, x, y, (
                (roller->direction == df::screw_pump_direction::FromNorth)? "Southward":
                (roller->direction == df::screw_pump_direction::FromEast)?  "Westward":
                (roller->direction == df::screw_pump_direction::FromSouth)? "Northward":
                (roller->direction == df::screw_pump_direction::FromWest)?  "Eastward":
                ""
            ), true, left_margin);

            OutputString(COLOR_LIGHTRED, x, y, Screen::getKeyDisplay(interface_key::BUILDING_ROLLERS_SPEED_DOWN));
            OutputString(COLOR_LIGHTRED, x, y, Screen::getKeyDisplay(interface_key::BUILDING_ROLLERS_SPEED_UP));
            OutputString(COLOR_WHITE, x, y, ": ");
            OutputString(COLOR_WHITE, x, y, (
                (roller->speed <= Speed::Lowest)? "Lowest":
                (roller->speed <= Speed::Low)?    "Low":
                (roller->speed <= Speed::Medium)? "Medium":
                (roller->speed <= Speed::High)?   "High":
                "Highest"
            ));
            OutputString(COLOR_WHITE, x, y, " Speed", true, left_margin);
            y += 1;
            OutputString(COLOR_GREY, x, y, "DFHack");
        }
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(trackstop_hook, feed);
IMPLEMENT_VMETHOD_INTERPOSE(trackstop_hook, render);
IMPLEMENT_VMETHOD_INTERPOSE(roller_hook, feed);
IMPLEMENT_VMETHOD_INTERPOSE(roller_hook, render);


DFhackCExport command_result plugin_enable(color_ostream& out, bool enable) {
    // Accept the "enable trackstop" / "disable trackstop" commands.
    if (enable != enabled) {
        if (!INTERPOSE_HOOK(trackstop_hook, feed).apply(enable) ||
                !INTERPOSE_HOOK(trackstop_hook, render).apply(enable) ||
                !INTERPOSE_HOOK(roller_hook, feed).apply(enable) ||
                !INTERPOSE_HOOK(roller_hook, render).apply(enable)) {
            out.printerr("Could not %s trackstop hooks!\n", enable? "insert": "remove");
            return CR_FAILURE;
        }

        enabled = enable;
    }

    return CR_OK;
}

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands) {
    return plugin_enable(out, AUTOENABLE);
}

DFhackCExport command_result plugin_shutdown(color_ostream &out) {
    return plugin_enable(out, false);
}
