/*
 * Autogems plugin.
 * Creates a new Workshop Order setting, automatically cutting rough gems.
 * For best effect, include "enable autogems" in your dfhack.init configuration.
 */

#include <fstream>

#include "uicommon.h"

#include "modules/Buildings.h"
#include "modules/Filesystem.h"
#include "modules/Gui.h"
#include "modules/Job.h"
#include "modules/World.h"

#include "df/building_workshopst.h"
#include "df/buildings_other_id.h"
#include "df/builtin_mats.h"
#include "df/general_ref_building_holderst.h"
#include "df/job.h"
#include "df/job_item.h"
#include "df/viewscreen_dwarfmodest.h"

#include "jsoncpp-ex.h"

#define CONFIG_KEY "autogems/config"
#define DELTA_TICKS 1200
#define MAX_WORKSHOP_JOBS 10

using namespace DFHack;

DFHACK_PLUGIN("autogems");
DFHACK_PLUGIN_IS_ENABLED(enabled);

REQUIRE_GLOBAL(plotinfo);
REQUIRE_GLOBAL(world);

typedef int32_t item_id;
typedef int32_t mat_index;
typedef std::map<mat_index, int> gem_map;

bool running = false;
std::set<mat_index> blacklist;

void add_task(mat_index gem_type, df::building_workshopst *workshop) {
    // Create a single task in the specified workshop.
    // Partly copied from Buildings::linkForConstruct(); perhaps a refactor is in order.

    auto ref = df::allocate<df::general_ref_building_holderst>();
    if (!ref) {
        std::cerr << "Could not allocate general_ref_building_holderst" << std::endl;
        return;
    }

    ref->building_id = workshop->id;

    auto item = new df::job_item();
    if (!item) {
        std::cerr << "Could not allocate job_item" << std::endl;
        return;
    }

    item->item_type = df::item_type::ROUGH;
    item->mat_type = df::builtin_mats::INORGANIC;
    item->mat_index = gem_type;
    item->quantity = 1;
    item->vector_id = df::job_item_vector_id::ROUGH;

    auto job = new df::job();
    if (!job) {
        std::cerr << "Could not allocate job" << std::endl;
        return;
    }

    job->job_type = df::job_type::CutGems;
    job->pos = df::coord(workshop->centerx, workshop->centery, workshop->z);
    job->mat_type = df::builtin_mats::INORGANIC;
    job->mat_index = gem_type;
    job->general_refs.push_back(ref);
    job->job_items.push_back(item);

    workshop->jobs.push_back(job);
    Job::linkIntoWorld(job);
}

void add_tasks(gem_map &gem_types, df::building_workshopst *workshop) {
    int slots = MAX_WORKSHOP_JOBS - workshop->jobs.size();
    if (slots <= 0) {
        return;
    }

    for (auto g = gem_types.begin(); g != gem_types.end() && slots > 0; ++g) {
        while (g->second > 0 && slots > 0) {
            add_task(g->first, workshop);
            g->second -= 1;
            slots -= 1;
        }
    }
}

bool valid_gem(df::item* item) {
    if (item->getType() != item_type::ROUGH) return false;
    if (item->getMaterial() != builtin_mats::INORGANIC) return false;
    if (item->flags.bits.in_job) return false;
    if (item->flags.bits.forbid) return false;
    if (item->flags.bits.dump) return false;
    if (item->flags.bits.owned) return false;
    if (item->flags.bits.trader) return false;
    if (item->flags.bits.hostile) return false;
    if (item->flags.bits.removed) return false;
    if (item->flags.bits.encased) return false;
    if (item->flags.bits.construction) return false;
    if (item->flags.bits.garbage_collect) return false;
    if (item->flags.bits.in_building) return false;
    if (blacklist.count(item->getMaterialIndex())) return false;
    return true;
}

void create_jobs() {
    // Creates jobs in Jeweler's Workshops as necessary.
    // Todo: Consider path availability?
    std::set<item_id> stockpiled;
    std::set<df::building_workshopst*> unlinked;
    gem_map available;

    for (df::building *building : world->buildings.other[df::buildings_other_id::WORKSHOP_JEWELER]) {
        auto workshop = virtual_cast<df::building_workshopst>(building);
        if (!workshop) {
            Core::printerr("autogems: invalid building %i (not a workshop)\n", building->id);
            continue;
        }
        auto links = workshop->profile.links.take_from_pile;

        if (workshop->construction_stage < 3) {
            // Construction in progress.
            continue;
        }

        if (workshop->jobs.size() == 1 && workshop->jobs[0]->job_type == df::job_type::DestroyBuilding) {
            // Queued for destruction.
            continue;
        }

        auto profile = workshop->getWorkshopProfile();
        if (profile && profile->blocked_labors[df::unit_labor::CUT_GEM]) {
            // workshop profile does not allow cut gem jobs (fixes #1263)
            continue;
        }

        if (links.size() > 0) {
            for (auto l = links.begin(); l != links.end() && workshop->jobs.size() <= MAX_WORKSHOP_JOBS; ++l) {
                auto stockpile = virtual_cast<df::building_stockpilest>(*l);
                gem_map piled;

                Buildings::StockpileIterator stored;
                for (stored.begin(stockpile); !stored.done(); ++stored) {
                    auto item = *stored;
                    if (valid_gem(item)) {
                        stockpiled.insert(item->id);
                        piled[item->getMaterialIndex()] += 1;
                    }
                    else if (item->flags.bits.container) {
                        std::vector<df::item*> binneditems;
                        Items::getContainedItems(item, &binneditems);
                        for (df::item *it : binneditems) {
                            if (valid_gem(it)) {
                                stockpiled.insert(it->id);
                                piled[it->getMaterialIndex()] += 1;
                            }
                        }
                    }
                }

                // Decrement current jobs from all linked workshops, not just this one.
                for (auto bld : stockpile->links.give_to_workshop) {
                    auto shop = virtual_cast<df::building_workshopst>(bld);
                    if (!shop) {
                        // e.g. furnace
                        continue;
                    }
                    for (auto job : shop->jobs) {
                        if (job->job_type == df::job_type::CutGems) {
                            if (job->flags.bits.repeat) {
                                piled[job->mat_index] = 0;
                            } else {
                                piled[job->mat_index] -= 1;
                            }
                        }
                    }
                }

                add_tasks(piled, workshop);
            }
        } else {
            // Note which gem types have already been ordered to be cut.
            for (auto j = workshop->jobs.begin(); j != workshop->jobs.end(); ++j) {
                auto job = *j;
                if (job->job_type == df::job_type::CutGems) {
                    available[job->mat_index] -= job->flags.bits.repeat? 100: 1;
                }
            }

            if (workshop->jobs.size() <= MAX_WORKSHOP_JOBS) {
                unlinked.insert(workshop);
            }
        }
    }

    if (unlinked.size() > 0) {
        // Count how many gems of each type are available to be cut.
        // Gems in stockpiles linked to specific workshops don't count.
        auto gems = world->items.other[items_other_id::ROUGH];
        for (auto g = gems.begin(); g != gems.end(); ++g) {
            auto item = *g;
            if (valid_gem(item) && !stockpiled.count(item->id)) {
                available[item->getMaterialIndex()] += 1;
            }
        }

        for (auto w = unlinked.begin(); w != unlinked.end(); ++w) {
            add_tasks(available, *w);
        }
    }
}

DFhackCExport command_result plugin_onupdate(color_ostream &out) {
    if (running && !World::ReadPauseState() && Maps::IsValid() &&
            (world->frame_counter % DELTA_TICKS == 0)) {
        create_jobs();
    }

    return CR_OK;
}


/*
 * Interface hooks
 */
struct autogem_hook : public df::viewscreen_dwarfmodest {
    typedef df::viewscreen_dwarfmodest interpose_base;

    bool in_menu() {
        // Determines whether we're looking at the Workshop Orders screen.
        return plotinfo->main.mode == ui_sidebar_mode::OrdersWorkshop;
    }

    bool handleInput(std::set<df::interface_key> *input) {
        if (!in_menu()) {
            return false;
        }

        if (input->count(interface_key::CUSTOM_G)) {
            // Toggle whether gems are auto-cut for this fort.
            auto config = World::GetPersistentData(CONFIG_KEY, NULL);
            if (config.isValid()) {
                config.ival(0) = running;
            }

            running = !running;
            return true;
        } else if (input->count(interface_key::CUSTOM_SHIFT_G)) {
            Core::getInstance().setHotkeyCmd("gui/autogems");
        }

        return false;
    }

    DEFINE_VMETHOD_INTERPOSE(void, feed, (std::set<df::interface_key> *input)) {
        if (!handleInput(input)) {
            INTERPOSE_NEXT(feed)(input);
        }
    }

    DEFINE_VMETHOD_INTERPOSE(void, render, ()) {
        INTERPOSE_NEXT(render)();
        if (in_menu()) {
            auto dims = Gui::getDwarfmodeViewDims();
            int x = dims.menu_x1 + 1;
            int y = dims.y1 + 12;
            Screen::Pen pen = Screen::readTile(x, y);

            while (pen.valid() && pen.ch != ' ') {
                pen = Screen::readTile(x, ++y);
            }

            if (pen.valid()) {
                OutputHotkeyString(x, y, (running? "Auto Cut Gems": "No Auto Cut Gems"),
                    interface_key::CUSTOM_G, false, x, COLOR_WHITE, COLOR_LIGHTRED);
                x += running ? 5 : 2;
                OutputHotkeyString(x, y, "Opts", interface_key::CUSTOM_SHIFT_G,
                    false, 0, COLOR_WHITE, COLOR_LIGHTRED);
            }
        }
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(autogem_hook, feed);
IMPLEMENT_VMETHOD_INTERPOSE(autogem_hook, render);

bool read_config(color_ostream &out) {
    std::string path = "save/" + World::ReadWorldFolder() + "/autogems.json";
    if (!Filesystem::isfile(path)) {
        // no need to require the config file to exist
        return true;
    }

    std::ifstream f(path);
    Json::Value config;
    try {
        if (!f.good() || !(f >> config)) {
            out.printerr("autogems: failed to read autogems.json\n");
            return false;
        }
    }
    catch (Json::Exception &e) {
        out.printerr("autogems: failed to read autogems.json: %s\n", e.what());
        return false;
    }

    if (config["blacklist"].isArray()) {
        blacklist.clear();
        for (int i = 0; i < int(config["blacklist"].size()); i++) {
            Json::Value item = config["blacklist"][i];
            if (item.isInt()) {
                blacklist.insert(mat_index(item.asInt()));
            }
            else {
                out.printerr("autogems: illegal item at position %i in blacklist\n", i);
            }
        }
    }
    return true;
}

command_result cmd_reload_config(color_ostream &out, std::vector<std::string>&) {
    return read_config(out) ? CR_OK : CR_FAILURE;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event) {
    if (event == DFHack::SC_MAP_LOADED) {
        if (enabled && World::isFortressMode()) {
            // Determine whether auto gem cutting has been disabled for this fort.
            auto config = World::GetPersistentData(CONFIG_KEY);
            running = config.isValid() && !config.ival(0);
            read_config(out);
        }
    } else if (event == DFHack::SC_MAP_UNLOADED) {
        running = false;
    }

    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream& out, bool enable) {
    if (enable != enabled) {
        if (!INTERPOSE_HOOK(autogem_hook, feed).apply(enable) || !INTERPOSE_HOOK(autogem_hook, render).apply(enable)) {
            out.printerr("Could not %s autogem hooks!\n", enable? "insert": "remove");
            return CR_FAILURE;
        }

        enabled = enable;
    }

    running = enabled && World::isFortressMode();
    if (running) {
        read_config(out);
    }
    return CR_OK;
}

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands) {
    commands.push_back(PluginCommand(
        "autogems-reload",
        "Reload autogems config file.",
        cmd_reload_config));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out) {
    return plugin_enable(out, false);
}
