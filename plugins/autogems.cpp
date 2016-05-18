/*
 * Autogems plugin.
 * Creates a new Workshop Order setting, automatically cutting rough gems.
 * For best effect, include "enable autogems" in your dfhack.init configuration.
 */

#include "uicommon.h"

#include "modules/Buildings.h"
#include "modules/Gui.h"
#include "modules/Job.h"
#include "modules/Persistent.h"
#include "modules/World.h"

#include "df/building_workshopst.h"
#include "df/buildings_other_id.h"
#include "df/builtin_mats.h"
#include "df/general_ref_building_holderst.h"
#include "df/job.h"
#include "df/job_item.h"
#include "df/viewscreen_dwarfmodest.h"

#define CONFIG_KEY "autogems/config"
#define DELTA_TICKS 1200
#define MAX_WORKSHOP_JOBS 10
static const int32_t persist_version = 1;

using namespace DFHack;

DFHACK_PLUGIN("autogems");
DFHACK_PLUGIN_IS_ENABLED(enabled);

REQUIRE_GLOBAL(ui);
REQUIRE_GLOBAL(world);

typedef int32_t item_id;
typedef int32_t mat_index;
typedef std::map<mat_index, int> gem_map;

bool running = false;
decltype(world->frame_counter) last_frame_count = 0;
const char *tagline = "Creates a new Workshop Order setting, automatically cutting rough gems.";
const char *usage = (
    "  enable autogems\n"
    "    Enable the plugin.\n"
    "  disable autogems\n"
    "    Disable the plugin.\n"
    "\n"
    "While enabled, the Current Workshop Orders screen (o-W) have a new option:\n"
    "  g: Auto Cut Gems\n"
    "\n"
    "While this option is enabled, jobs will be created in Jeweler's Workshops\n"
    "to cut any accessible rough gems.\n"
);

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
    return true;
}

void create_jobs() {
    // Creates jobs in Jeweler's Workshops as necessary.
    // Todo: Consider path availability?
    std::set<item_id> stockpiled;
    std::set<df::building_workshopst*> unlinked;
    gem_map available;
    auto workshops = &world->buildings.other[df::buildings_other_id::WORKSHOP_JEWELER];

    for (auto w = workshops->begin(); w != workshops->end(); ++w) {
        auto workshop = virtual_cast<df::building_workshopst>(*w);
        auto links = workshop->profile.links.take_from_pile;

        if (workshop->construction_stage < 3) {
            // Construction in progress.
            continue;
        }

        if (workshop->jobs.size() == 1 && workshop->jobs[0]->job_type == df::job_type::DestroyBuilding) {
            // Queued for destruction.
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
                }

                // Decrement current jobs from all linked workshops, not just this one.
                auto outbound = stockpile->links.give_to_workshop;
                for (auto ws = outbound.begin(); ws != outbound.end(); ++ws) {
                    auto shop = virtual_cast<df::building_workshopst>(*ws);
                    for (auto j = shop->jobs.begin(); j != shop->jobs.end(); ++j) {
                        auto job = *j;
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

void save_config() {
    Json::Value& data = Persistent::get("autogems");
    data["version"] = persist_version;
    data["running"] = running;
}

void load_config(color_ostream &out) {
    Json::Value& data = Persistent::get("autogems");
    int32_t version = Json::get<int>(data, "version", 0);
    if ( version == 0 ) {
        // Load from historical figure.
        // Always saves back to the property_tree,
        // to avoid looking up the histfig on every load.
        PersistentDataItem config = World::GetPersistentData(CONFIG_KEY);
        running = config.isValid() && !config.ival(0);
        save_config();
        World::DeletePersistentData(config);
    } else if ( version == 1 ) {
        // Load from the property_tree.
        running = Json::get<bool>(data, "running", false);
    } else {
        // Collect what we can, but warn about the unknown version.
        // It's probably fine, but could mean that the user expected a
        // newer plugin version, with features currently unimagined.
        out.printerr("autogems: Unknown configuration version %d\n", version);
        running = Json::get<bool>(data, "running", false);
    }
}

DFhackCExport command_result plugin_onupdate(color_ostream &out) {
    if (running && (world->frame_counter - last_frame_count >= DELTA_TICKS)) {
        last_frame_count = world->frame_counter;
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
        return ui->main.mode == ui_sidebar_mode::OrdersWorkshop;
    }

    bool handleInput(std::set<df::interface_key> *input) {
        if (!in_menu()) {
            return false;
        }

        if (input->count(interface_key::CUSTOM_G)) {
            // Toggle whether gems are auto-cut for this fort.
            running = !running;
            save_config();
            return true;
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
                OutputHotkeyString(x, y, (running? "Auto Cut Gems": "No Auto Cut Gems"), "g", false, x, COLOR_WHITE, COLOR_LIGHTRED);
            }
        }
    }
};

IMPLEMENT_VMETHOD_INTERPOSE(autogem_hook, feed);
IMPLEMENT_VMETHOD_INTERPOSE(autogem_hook, render);


DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event) {
    if (event == DFHack::SC_MAP_LOADED) {
        if (enabled && World::isFortressMode()) {
            // Determine whether auto gem cutting has been disabled for this fort.
            load_config(out);
            last_frame_count = world->frame_counter;
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
    return CR_OK;
}

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands) {
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out) {
    return plugin_enable(out, false);
}
