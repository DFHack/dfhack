#include "pause.h"

#include <Debug.h>
#include <Core.h>
#include <Export.h>
#include <PluginManager.h>

#include <modules/EventManager.h>
#include <modules/World.h>
#include <modules/Maps.h>
#include <modules/Gui.h>
#include <modules/Job.h>
#include <modules/Units.h>

#include <df/job.h>
#include <df/unit.h>
#include <df/historical_figure.h>
#include <df/global_objects.h>
#include <df/world.h>
#include <df/viewscreen.h>
#include <df/creature_raw.h>

#include <array>
#include <random>
#include <cinttypes>

// Debugging
namespace DFHack {
    DBG_DECLARE(log, plugin, DebugCategory::LINFO);
}

DFHACK_PLUGIN("spectate");
DFHACK_PLUGIN_IS_ENABLED(enabled);

REQUIRE_GLOBAL(world);
REQUIRE_GLOBAL(plotinfo);
REQUIRE_GLOBAL(d_init); // used in pause.cpp

using namespace DFHack;
using namespace Pausing;
using namespace df::enums;

struct Configuration {
    bool unpause = false;
    bool disengage = false;
    bool animals = false;
    bool hostiles = true;
    bool visitors = false;
    int32_t tick_threshold = 1000;
} config;

Pausing::AnnouncementLock* pause_lock = nullptr;
bool lock_collision = false;
bool announcements_disabled = false;

#define base 0.99

static const std::string CONFIG_KEY = std::string(plugin_name) + "/config";
enum ConfigData {
    UNPAUSE,
    DISENGAGE,
    TICK_THRESHOLD,
    ANIMALS,
    HOSTILES,
    VISITORS
};

static PersistentDataItem pconfig;

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable);
command_result spectate (color_ostream &out, std::vector <std::string> & parameters);
#define COORDARGS(id) id.x, id.y, id.z

namespace SP {
    bool following_dwarf = false;
    df::unit* our_dorf = nullptr;
    int32_t timestamp = -1;
    std::default_random_engine RNG;

    void DebugUnitVector(std::vector<df::unit*> units) {
        if (debug_plugin.isEnabled(DFHack::DebugCategory::LDEBUG)) {
            for (auto unit: units) {
                DEBUG(plugin).print("[id: %d]\n animal: %d\n hostile: %d\n visiting: %d\n",
                                    unit->id,
                                    Units::isAnimal(unit),
                                    Units::isDanger(unit),
                                    Units::isVisiting(unit));
            }
        }
    }

    void PrintStatus(color_ostream &out) {
        out.print("Spectate is %s\n", enabled ? "ENABLED." : "DISABLED.");
        out.print(" FEATURES:\n");
        out.print("  %-20s\t%s\n", "auto-unpause: ", config.unpause ? "on." : "off.");
        out.print("  %-20s\t%s\n", "auto-disengage: ", config.disengage ? "on." : "off.");
        out.print("  %-20s\t%s\n", "animals: ", config.animals ? "on." : "off.");
        out.print("  %-20s\t%s\n", "hostiles: ", config.hostiles ? "on." : "off.");
        out.print("  %-20s\t%s\n", "visiting: ", config.visitors ? "on." : "off.");
        out.print(" SETTINGS:\n");
        out.print("  %-20s\t%" PRIi32 "\n", "tick-threshold: ", config.tick_threshold);
        if (following_dwarf)
            out.print(" %-21s\t%s[id: %d]\n","FOLLOWING:", our_dorf ? our_dorf->name.first_name.c_str() : "nullptr", plotinfo->follow_unit);
    }

    void SetUnpauseState(bool state) {
        // we don't need to do any of this yet if the plugin isn't enabled
        if (enabled) {
            // todo: R.E. UNDEAD_ATTACK event [still pausing regardless of announcement settings]
            // lock_collision == true means: enable_auto_unpause() was already invoked and didn't complete
            // The onupdate function above ensure the procedure properly completes, thus we only care about
            // state reversal here ergo `enabled != state`
            if (lock_collision && config.unpause != state) {
                WARN(plugin).print("Spectate auto-unpause: Not enabled yet, there was a lock collision. When the other lock holder releases, auto-unpause will engage on its own.\n");
                // if unpaused_enabled is true, then a lock collision means: we couldn't save/disable the pause settings,
                // therefore nothing to revert and the lock won't even be engaged (nothing to unlock)
                lock_collision = false;
                config.unpause = state;
                if (config.unpause) {
                    // a collision means we couldn't restore the pause settings, therefore we only need re-engage the lock
                    pause_lock->lock();
                }
                return;
            }
            // update the announcement settings if we can
            if (state) {
                if (World::SaveAnnouncementSettings()) {
                    World::DisableAnnouncementPausing();
                    announcements_disabled = true;
                    pause_lock->lock();
                } else {
                    WARN(plugin).print("Spectate auto-unpause: Could not fully enable. There was a lock collision, when the other lock holder releases, auto-unpause will engage on its own.\n");
                    lock_collision = true;
                }
            } else {
                pause_lock->unlock();
                if (announcements_disabled) {
                    if (!World::RestoreAnnouncementSettings()) {
                        // this in theory shouldn't happen, if others use the lock like we do in spectate
                        WARN(plugin).print("Spectate auto-unpause: Could not fully disable. There was a lock collision, when the other lock holder releases, auto-unpause will disengage on its own.\n");
                        lock_collision = true;
                    } else {
                        announcements_disabled = false;
                    }
                }
            }
            if (lock_collision) {
                ERR(plugin).print("Spectate auto-unpause: Could not fully enable. There was a lock collision, when the other lock holder releases, auto-unpause will engage on its own.\n");
                WARN(plugin).print(
                            " auto-unpause: must wait for another Pausing::AnnouncementLock to be lifted.\n"
                            " The action you were attempting will complete when the following lock or locks lift.\n");
                pause_lock->reportLocks(Core::getInstance().getConsole());
            }
        }
        config.unpause = state;
    }

    void SaveSettings() {
        if (pconfig.isValid()) {
            pconfig.ival(UNPAUSE) = config.unpause;
            pconfig.ival(DISENGAGE) = config.disengage;
            pconfig.ival(TICK_THRESHOLD) = config.tick_threshold;
            pconfig.ival(ANIMALS) = config.animals;
            pconfig.ival(HOSTILES) = config.hostiles;
            pconfig.ival(VISITORS) = config.visitors;
        }
    }

    void LoadSettings() {
        pconfig = World::GetPersistentSiteData(CONFIG_KEY);

        if (!pconfig.isValid()) {
            pconfig = World::AddPersistentSiteData(CONFIG_KEY);
            SaveSettings();
        } else {
            config.unpause = pconfig.ival(UNPAUSE);
            config.disengage = pconfig.ival(DISENGAGE);
            config.tick_threshold = pconfig.ival(TICK_THRESHOLD);
            config.animals = pconfig.ival(ANIMALS);
            config.hostiles = pconfig.ival(HOSTILES);
            config.visitors = pconfig.ival(VISITORS);
            pause_lock->unlock();
            SetUnpauseState(config.unpause);
        }
    }

    bool FollowADwarf() {
        if (enabled && !World::ReadPauseState()) {
            df::coord viewMin = Gui::getViewportPos();
            df::coord viewMax{viewMin};
            const auto &dims = Gui::getDwarfmodeViewDims().map().second;
            viewMax.x += dims.x - 1;
            viewMax.y += dims.y - 1;
            viewMax.z = viewMin.z;
            std::vector<df::unit*> units;
            static auto add_if = [&](std::function<bool(df::unit*)> check) {
                for (auto unit : world->units.active) {
                    if (check(unit)) {
                        units.push_back(unit);
                    }
                }
            };
            static auto valid = [](df::unit* unit) {
                if (Units::isAnimal(unit)) {
                    return config.animals;
                }
                if (Units::isVisiting(unit)) {
                    return config.visitors;
                }
                if (Units::isDanger(unit)) {
                    return config.hostiles;
                }
                return true;
            };
            static auto calc_extra_weight = [](size_t idx, double r1, double r2) {
                switch(idx) {
                    case 0:
                        return r2;
                    case 1:
                        return (r2-r1)/1.3;
                    case 2:
                        return (r2-r1)/2;
                    default:
                        return 0.0;
                }
            };
            /// Collecting our choice pool
            ///////////////////////////////
            std::array<int32_t, 10> ranges{};
            std::array<bool, 5> range_exists{};
            static auto build_range = [&](size_t idx){
                size_t first = idx * 2;
                size_t second = idx * 2 + 1;
                size_t previous = first - 1;
                // first we get the end of the range
                ranges[second] = units.size() - 1;
                // then we calculate whether the range exists, and set the first index appropriately
                if (idx == 0) {
                    range_exists[idx] = ranges[second] >= 0;
                    ranges[first] = 0;
                } else {
                    range_exists[idx] = ranges[second] > ranges[previous];
                    ranges[first] = ranges[previous] +  (range_exists[idx] ? 1 : 0);
                }
            };

            /// RANGE 0 (in view + working)
            // grab valid working units
            add_if([&](df::unit* unit) {
                return valid(unit) &&
                       Units::isUnitInBox(unit, COORDARGS(viewMin), COORDARGS(viewMax)) &&
                       Units::isCitizen(unit, true) &&
                       unit->job.current_job;
            });
            build_range(0);

            /// RANGE 1 (in view)
            add_if([&](df::unit* unit) {
                return valid(unit) && Units::isUnitInBox(unit, COORDARGS(viewMin), COORDARGS(viewMax));
            });
            build_range(1);

            /// RANGE 2 (working citizens)
            add_if([](df::unit* unit) {
                return valid(unit) && Units::isCitizen(unit, true) && unit->job.current_job;
            });
            build_range(2);

            /// RANGE 3 (citizens)
            add_if([](df::unit* unit) {
                return valid(unit) && Units::isCitizen(unit, true);
            });
            build_range(3);

            /// RANGE 4 (any valid)
            add_if(valid);
            build_range(4);

            // selecting from our choice pool
            if (!units.empty()) {
                std::array<double, 5> bw{23,17,13,7,1}; // probability weights for each range
                std::vector<double> i;
                std::vector<double> w;
                bool at_least_one = false;
                // in one word, elegance
                for(size_t idx = 0; idx < range_exists.size(); ++idx) {
                    if (range_exists[idx]) {
                        at_least_one = true;
                        const auto &r1 = ranges[idx*2];
                        const auto &r2 = ranges[idx*2+1];
                        double extra = calc_extra_weight(idx, r1, r2);
                        i.push_back(r1);
                        w.push_back(bw[idx] + extra);
                        if (r1 != r2) {
                            i.push_back(r2);
                            w.push_back(bw[idx] + extra);
                        }
                    }
                }
                if (!at_least_one) {
                    return false;
                }
                DebugUnitVector(units);
                std::piecewise_linear_distribution<> follow_any(i.begin(), i.end(), w.begin());
                // if you're looking at a warning about a local address escaping, it means the unit* from units (which aren't local)
                size_t idx = follow_any(RNG);
                our_dorf = units[idx];
                plotinfo->follow_unit = our_dorf->id;
                timestamp = world->frame_counter;
                return true;
            } else {
                WARN(plugin).print("units vector is empty!\n");
            }
        }
        return false;
    }

    void onUpdate(color_ostream &out) {
        // keeps announcement pause settings locked
        World::Update(); // from pause.h

        // Plugin Management
        if (lock_collision) {
            if (config.unpause) {
                // player asked for auto-unpause enabled
                World::SaveAnnouncementSettings();
                if (World::DisableAnnouncementPausing()) {
                    // now that we've got what we want, we can lock it down
                    lock_collision = false;
                }
            } else {
                if (World::RestoreAnnouncementSettings()) {
                    lock_collision = false;
                }
            }
        }
        int failsafe = 0;
        while (config.unpause && !world->status.popups.empty() && ++failsafe <= 10) {
            // dismiss announcement popup(s)
            Gui::getCurViewscreen(true)->feed_key(interface_key::CLOSE_MEGA_ANNOUNCEMENT);
            if (World::ReadPauseState()) {
                // WARNING: This has a possibility of conflicting with `reveal hell` - if Hermes himself runs `reveal hell` on precisely the right moment that is
                World::SetPauseState(false);
            }
        }
        if (failsafe >= 10) {
            out.printerr("spectate encountered a problem dismissing a popup!\n");
        }

        // plugin logic
        static int32_t last_tick = -1;
        int32_t tick = world->frame_counter;
        if (!World::ReadPauseState() && tick - last_tick >= 1) {
            last_tick = tick;
            // validate follow state
            if (!following_dwarf || !our_dorf || plotinfo->follow_unit < 0 || tick - timestamp >= config.tick_threshold) {
                // we're not following anyone
                following_dwarf = false;
                if (!config.disengage) {
                    // try to
                    following_dwarf = FollowADwarf();
                } else if (!World::ReadPauseState()) {
                    plugin_enable(out, false);
                }
            }
        }
    }
};

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands) {
    commands.push_back(PluginCommand("spectate",
                                     "Automated spectator mode.",
                                     spectate,
                                     false));
    pause_lock = new AnnouncementLock("spectate");
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out) {
    delete pause_lock;
    return CR_OK;
}

DFhackCExport command_result plugin_load_site_data (color_ostream &out) {
    SP::LoadSettings();
    if (enabled) {
        SP::following_dwarf = SP::FollowADwarf();
        SP::PrintStatus(out);
    }
    return DFHack::CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable) {
    if (!Core::getInstance().isMapLoaded() || !World::IsSiteLoaded()) {
        out.printerr("Cannot run %s without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }

    if (enable && !enabled) {
        out.print("Spectate mode enabled!\n");
        enabled = true; // enable_auto_unpause won't do anything without this set now
        SP::SetUnpauseState(config.unpause);
    } else if (!enable && enabled) {
        // warp 8, engage!
        out.print("Spectate mode disabled!\n");
        // we need to retain whether auto-unpause is enabled, but we also need to disable its effect
        bool temp = config.unpause;
        SP::SetUnpauseState(false);
        config.unpause = temp;
    }
    enabled = enable;
    return DFHack::CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event) {
    if (enabled) {
        switch (event) {
            case SC_WORLD_UNLOADED:
                SP::our_dorf = nullptr;
                SP::following_dwarf = false;
                enabled = false;
            default:
                break;
        }
    }
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate(color_ostream &out) {
    SP::onUpdate(out);
    return DFHack::CR_OK;
}

command_result spectate (color_ostream &out, std::vector <std::string> & parameters) {
    if (!Core::getInstance().isMapLoaded() || !World::IsSiteLoaded()) {
        out.printerr("Cannot run %s without a loaded fort.\n", plugin_name);
        return CR_FAILURE;
    }

    if (!parameters.empty()) {
        if (parameters.size() >= 2 && parameters.size() <= 3) {
            bool state =false;
            bool set = false;
            if (parameters[0] == "enable") {
                state = true;
            } else if (parameters[0] == "disable") {
                state = false;
            } else if (parameters[0] == "set") {
                set = true;
            } else {
                return DFHack::CR_WRONG_USAGE;
            }
            if(parameters[1] == "auto-unpause"){
                SP::SetUnpauseState(state);
            } else if (parameters[1] == "auto-disengage") {
                config.disengage = state;
            } else if (parameters[1] == "animals") {
                config.animals = state;
            } else if (parameters[1] == "hostiles") {
                config.hostiles = state;
            } else if (parameters[1] == "visiting") {
                config.visitors = state;
            } else if (parameters[1] == "tick-threshold" && set && parameters.size() == 3) {
                try {
                    config.tick_threshold = std::abs(std::stol(parameters[2]));
                } catch (const std::exception &e) {
                    out.printerr("%s\n", e.what());
                }
            } else {
                return DFHack::CR_WRONG_USAGE;
            }
        }
    } else {
        SP::PrintStatus(out);
    }
    SP::SaveSettings();
    return DFHack::CR_OK;
}
